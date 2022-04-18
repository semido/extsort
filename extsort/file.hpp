#pragma once
#include "timer.hpp"
#include <cstdio>
#include <string>
using namespace std::string_literals;
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class File
{
  FILE* fp = nullptr;
public:
  File() = default;
  File(const std::string& name, const std::string& mode)
  {
    fp = std::fopen(name.data(), mode.data());
  }
  ~File()
  {
    close();
  }
  operator FILE* ()
  {
    return fp;
  }
  operator bool()
  {
    if (fp)
      return !std::ferror(fp);
    return false;
  }
  bool open(const std::string& name, const std::string& mode)
  {
    fp = std::fopen(name.data(), mode.data());
    return *this;
  }
  void close()
  {
    if (fp)
      std::fclose(fp);
    fp = nullptr;
  }
  size_t size()
  {
    auto pos = std::ftell(fp);
    std::fseek(fp, 0, SEEK_END);
    auto fileSize = std::ftell(fp);
    std::fseek(fp, pos, SEEK_SET);
    return fileSize;
  }
  template<class T>
  auto read(T& v)
  {
    return std::fread(&v, sizeof(T), 1, fp);
  }
  template<class T>
  auto read(T* p, size_t sz)
  {
    return std::fread(p, sizeof(T), sz, fp);
  }
  template<class T>
  auto read(std::vector<T>& v)
  {
    return std::fread(v.data(), sizeof(T), v.size(), fp);
  }
  template<class T>
  auto write(T* p, size_t sz)
  {
    return std::fwrite(p, sizeof(T), sz, fp);
  }
  template<class T>
  auto write(const std::vector<T>& v)
  {
    return std::fwrite(v.data(), sizeof(T), v.size(), fp);
  }
};

// Helper for std::vector<NoInit<T>>
template<class T>
class NoInit {
public:
  NoInit() {}
  constexpr  NoInit(T v) : value(v) {}
  constexpr  operator T () const { return value; }
private:
  T value;
};

// Double buffered write.
// Main thread is collecting.
// Additional thread is to flush into the file.
// TODO: list<bufs>
template<class T>
class FileWriteBuf
{
public:

  FileWriteBuf(const std::string& name, size_t sz = 256*1024) :
    file(name, "wb"s),
    t(&FileWriteBuf::run, this)
  {
    buf.reserve(sz);
    bufWrite.reserve(sz);
  }

  ~FileWriteBuf()
  {
    {
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [this] { return ! doFlush; });
    }
    isTerminating = true;
    swap();
    t.join();
  }

  void push_back(const T x)
  {
    bool resized = buf.size() == buf.capacity();
    buf.push_back(x);
    if (! resized && 2 * buf.size() < buf.capacity())
      return;
    Timer timer;
    {
      std::lock_guard<std::mutex> lock(m);
      if (doFlush) {
        if (resized)
          maxCapacity = std::max(maxCapacity, buf.capacity());
        return;
      }
    }
    swap();
    mainThreadWaits += timer;
  }

  auto wasResized() const { return maxCapacity; }

  auto mainWaits() const { return mainThreadWaits; }

private:

  void run()
  {
    while (true) {
      {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this] { return doFlush; });
      }
      if (bufWrite.size()) {
        file.write(bufWrite);
        bufWrite.clear();
      }
      {
        std::lock_guard<std::mutex> lock(m);
        doFlush = false;
        if (isTerminating)
          break;
      }
      cv.notify_all(); // NB: dtor may wait it.
    }
  }

  void swap()
  {
    {
      std::lock_guard<std::mutex> lock(m);
      std::swap(buf, bufWrite);
      doFlush = true;
    }
    cv.notify_one(); // Flushing thread waits it.
  }

  File file;

  std::thread t;
  std::mutex m;
  std::condition_variable cv;
  bool doFlush = false;
  bool isTerminating = false;
  size_t maxCapacity = 0;
  double mainThreadWaits = 0.0;

  std::vector<NoInit<T>> buf; // Collect by push_back.
  std::vector<NoInit<T>> bufWrite; // Write it to file.
};

// Double buffered read.
// Main thread is read from buf. Additional thread is to load bufs from the file.
template<class T>
class FileReadBuf
{
public:

  FileReadBuf(const std::string& name, size_t sz = 256 * 1024) :
    file(name, "rb"s),
    t(&FileReadBuf::run, this),
    isEOF(false)
  {
    buf.reserve(sz);
    buf2.reserve(sz);
    setLoad(true);
  }

  FileReadBuf(const FileReadBuf&) {}

  ~FileReadBuf()
  {
    {
      std::lock_guard<std::mutex> lock(m);
      isEOF = true;
    }
    cv.notify_one();
    t.join();
  }

  size_t size() { return file.size(); }

  bool read(T& x)
  {
    if (bufpos >= buf.size()) {
      if (isEOF)
        return false;
      Timer timer;
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [this] { return !doLoad; });
      //lock.unlock();
      buf.clear();
      bufpos = 0;
      if (isEOF)
        return false;
      std::swap(buf, buf2);
      lock.unlock();
      setLoad(true);
      mainThreadWaits += timer;
    }
    x = buf[bufpos++];
    return true;
  }

  auto mainWaits() const { return mainThreadWaits; }

private:
  template <class Pred>
  void wait(Pred pred)
  {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, pred);
  }
  void setLoad(bool b)
  {
    {
      std::lock_guard<std::mutex> lock(m);
      doLoad = b;
    }
    cv.notify_one();
  }
  void run()
  {
    while (true)
    {
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [this] { return doLoad || isEOF; });
      if (isEOF)
        break;
      lock.unlock();
      buf2.resize(buf2.capacity());
      buf2.resize(file.read(buf2));
      if (buf2.size() == 0)
        isEOF = true;
      setLoad(false);
    }
  }

  File file;

  std::thread t;
  std::mutex m;
  std::condition_variable cv;
  bool doLoad = false;
  std::atomic<bool> isEOF;
  double mainThreadWaits = 0.0;

  size_t bufpos = 0;
  std::vector<NoInit<T>> buf;
  std::vector<NoInit<T>> buf2;
};
