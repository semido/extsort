#pragma once
#include <string>
#include <vector>

void mergeFiles(const std::string& output, const std::vector<int>& ids);

inline void mergeFiles1(std::vector<int>& ids)
{
  std::string output = std::to_string(ids.back());
  ids.pop_back();
  mergeFiles(output, ids);
}
