#include "extsort.hpp"
#include "thread_pool.hpp"
#include "file.hpp"
#include "merge.hpp"
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>

#define USE_THREADS 1

typedef unsigned Data;
typedef std::vector<NoInit<Data>> Buffer;
struct Chunk
{
  int uid;
  Buffer buf;
};

static void sortOnePiece(Chunk& chunk)
{
  std::sort(chunk.buf.begin(), chunk.buf.end());
  File f(std::to_string(chunk.uid), "wb"s);
  f.write(chunk.buf);
}

static int createSortedPieces(
  const std::string& input,
  size_t memSize,
  int numThreads
)
{
  File f(input, "rb"s);
  size_t fileSize = f.size(); // Bytes.
  size_t pieces = std::ceil(double(fileSize) / (memSize / numThreads));
  size_t bufSize = (fileSize / sizeof(Data)) / pieces + 1; // Numbers.
  std::cout << "file size = " << fileSize << "(" << double(fileSize) / (1024.0 * 1024.0) << "M),";
  std::cout << " mem size = " << memSize << "(" << double(memSize) / (1024.0 * 1024.0) << "M),";
  std::cout << " buf size = " << bufSize << ",";
  std::cout << " pieces = " << double(fileSize) / (bufSize * sizeof(Data)) << "\n";
#ifdef USE_THREADS
  ThreadPool<Chunk, decltype(sortOnePiece)> pool(numThreads, sortOnePiece);
#endif
  int uid = 0;
  for (; uid * bufSize * sizeof(Data) < fileSize; uid++) {
#ifdef USE_THREADS
    auto& t = pool.waitFree();
    auto& chunk = t.setup();
    chunk.uid = uid;
#else
    Chunk chunk;
#endif
    chunk.buf.resize(bufSize);
    auto loadedSize = f.read(chunk.buf);
    chunk.buf.resize(loadedSize);
#ifdef USE_THREADS
    t.start();
#else
    sortOnePiece(chunk);
#endif
    if (loadedSize < bufSize) {
      uid++;
      break;
    }
  }
  return uid;
}

static void straightMergeFiles(
  const std::string& output, 
  int n
)
{
  File o(output, "wb"s);
  for (int i = 0; i < n; i++) {
    auto name = std::to_string(i);
    File f(name, "rb"s);
    auto fileSize = f.size();
    auto bufSize = fileSize / sizeof(Data);
    Buffer data(bufSize);
    auto loadedSize = f.read(data);
    data.resize(loadedSize);
    o.write(data);
    f.close();
    std::remove(name.data());
  }
}

void externalMerge(
  const std::string& output,
  int nFiles,
  int numSlots
)
{
  if (numSlots == 0)
    numSlots = nFiles;
  std::cout << "Merge slots: " << numSlots << "\n";
  std::vector<int> ids(nFiles);
  std::iota(ids.begin(), ids.end(), 0);
  while (numSlots < ids.size()) {
    std::vector<int> ids2(ids.begin(), ids.begin() + numSlots);
    ids.erase(ids.begin(), ids.begin() + numSlots);
    ids.push_back(nFiles++);
    mergeFiles(std::to_string(ids.back()), ids2);
  }
  mergeFiles(output, ids);
  std::cout << "Intermediate files: " << nFiles << "\n";
}

void externalSort(
  const std::string& input,
  const std::string& output,
  size_t memSize,
  int numThreads,
  int numSlots
)
{
  auto nFiles = createSortedPieces(input, memSize, numThreads);
  externalMerge(output, nFiles, numSlots);
}

void externalSortNPasses(
  const std::string& input,
  const std::string& output,
  size_t memSize,
  int numThreads,
  int numPasses
)
{
  auto nFiles = createSortedPieces(input, memSize, numThreads);
  if (numPasses == 0 || numPasses * 2 >= nFiles)
    externalMerge(output, nFiles, 0);
  else
    externalMerge(output, nFiles, nFiles / numPasses + 1);
  
}
