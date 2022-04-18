#pragma once
#include <chrono>

class Timer // seconds
{
public:
  typedef std::chrono::high_resolution_clock clock;
  Timer()
  {
    t0 = clock::now();
  }
  operator double()
  {
    auto t1 = clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
  }
private:
  clock::time_point t0;
};
