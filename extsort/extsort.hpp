#pragma once
#include <string>

void externalSort(
  const std::string& input,
  const std::string& output,
  size_t memSize,
  int numThreads,
  int slots
);

void externalSortNPasses(
  const std::string& input,
  const std::string& output,
  size_t memSize,
  int numThreads,
  int numPasses
);
