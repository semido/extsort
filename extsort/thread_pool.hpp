#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>

template<class Data, class Function>
class ThreadPool
{
  friend class Thread;
  template<class Data1 = Data, class Function1 = Function>
  class Thread
  {
    ThreadPool<Data, Function>& pool;
    Function* func;
    Data data;
    bool dataLoaded = false;

  public:

    Thread(ThreadPool<Data, Function>& tp, Function f) : pool(tp), func(f) {}

    Data& setup()
    {
      return data;
    }

    void start()
    {
      pool.notify([this]() { setLoaded(true); });
    }

  private:
    friend class ThreadPool;

    // Call this only over 'pool.notify()'!
    void setLoaded(bool b)
    {
      dataLoaded = b;
    }

    bool isLoaded() const
    {
      return dataLoaded;
    }

    void run()
    {
      while (true) {
        pool.wait([this] { return isLoaded() || pool.isTerminating(); });
        if (pool.isTerminating())
          break;
        func(data);
        pool.notify([this]() { setLoaded(false); });
      }
    }
  };

  std::mutex m;
  std::condition_variable cv;
  std::atomic<bool> terminating;

  std::vector<Thread<Data, Function>> datas;
  std::vector<std::thread> threads;

public:

  ThreadPool(int sz, Function f)
  {
    terminating = false;
    datas.reserve(sz);
    threads.reserve(sz);
    for (int t = 0; t < sz; t++) {
      datas.emplace_back(*this, f);
      threads.emplace_back(&Thread<Data, Function>::run, &datas[t]);
    }
  }

  ~ThreadPool()
  {
    terminate();
  }

  auto& waitFree()
  {
    int tid = -1;
    wait([this, &tid] {
      int busy = 0;
      for (int i = 0; i < datas.size(); i++)
        if (!datas[i].isLoaded())
          tid = i;
        else
          busy++;
      //std::cout << "busy " << busy << "\n";
      return tid != -1;
      });
    return datas[tid];
  }

  void terminate()
  {
    wait([this] {
      for (auto& d : datas)
        if (d.isLoaded())
          return false;
      return true;
      });
    notify([this]() { terminating = true; });
    for (auto& t : threads)
      t.join();
    threads.clear();
    datas.clear();
  }

  bool isTerminating() const
  {
    return terminating;
  }

private:

  template <class Modifier>
  void notify(Modifier func)
  {
    {
      std::lock_guard<std::mutex> lock(m);
      func();
    }
    cv.notify_all();
  }

  template <class Predicate>
  void wait(Predicate pred)
  {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, pred);
  }
};
