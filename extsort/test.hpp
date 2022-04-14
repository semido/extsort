#pragma once
#include "file.hpp"
#include <chrono>

class Timer // seconds
{
public:
  typedef std::chrono::steady_clock clock;
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

void generateSortedFile(const std::string& name, size_t size);

void generateFile1(const std::string& name, size_t size);

void doReferenceSort(const std::string& origName, const std::string& resName);

bool makeTest(const std::string& origName, const std::string& resName);
