#pragma once
#include <cstdio>
#include <string>
using namespace std::string_literals;
#include <vector>

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
  auto size()
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

// helper for std::vector<NoInit<?>>
template<class T>
class NoInit {
public:
  NoInit() {}
  constexpr  NoInit(T v) : value(v) {}
  constexpr  operator T () const { return value; }
private:
  T value;
};

template<class T>
class FileWriteBuf
{
public:
  FileWriteBuf(const std::string& name, size_t sz = 256) : file(name, "wb"s) { buf.reserve(sz); }
  ~FileWriteBuf() { writeBuf(); }
  void push_back(const T x)
  {
    buf.push_back(x);
    if (buf.size() == buf.capacity())
      writeBuf();
  }
private:
  File file;
  std::vector<NoInit<T>> buf;
  size_t bufpos = 0;
  void writeBuf()
  {
    file.write(buf);
    buf.clear();
  };
};

template<class T>
class FileReadBuf
{
public:
  FileReadBuf(const std::string& name, size_t sz = 256) : file(name, "rb"s) { buf.reserve(sz); }
  bool read(T& x)
  {
    if (bufpos >= buf.size()) {
      buf.resize(buf.capacity());
      auto sz = file.read(buf);
      bufpos = 0;
      buf.resize(sz);
      if (sz == 0)
        return false;
    }
    x = buf[bufpos++];
    return true;
  }

private:
  File file;
  std::vector<NoInit<T>> buf;
  size_t bufpos = 0;
};
