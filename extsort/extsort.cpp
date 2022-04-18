#include "extsort.hpp"
#include "thread_pool.hpp"
#include "file.hpp"
#include "merge.hpp"
#include "timer.hpp"
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
  Timer timer;
  File f(input, "rb"s);
  size_t fileSize = f.size(); // Bytes.
  size_t pieces = size_t(std::ceil(double(fileSize) / (memSize / numThreads)));
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
#ifdef USE_THREADS
  pool.terminate();
  std::cout << "Partial sort: " << timer << "sec. Main thread pool waits: " << pool.mainWaits() << "\n";
#endif
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

#ifdef USE_THREADS
void externalMergePar(
  const std::string& output,
  int nFiles,
  int numThreads
)
{
  Timer timerp;
  int nSlotsPerThread = std::max(2, int(double(nFiles) / numThreads + 0.5));
  std::vector<std::vector<int>> ids; // Split all input file ids for numThreads.
  std::vector<int> idsLast; // Ids for last pass.
  for (int i = 0; i < nFiles;) {
    int n = nSlotsPerThread;
    if (ids.size() + 1 == numThreads || nFiles - i < n + 2)
      n = nFiles - i;
    ids.emplace_back(n); // Last one for output file.
    std::iota(ids.back().begin(), ids.back().end(), i);
    idsLast.push_back(nFiles + int(idsLast.size()));
    ids.back().push_back(idsLast.back());
    i += n;
  }
  std::cout << "Merge threads: " << ids.size() << "\n";
  ThreadPool<std::vector<int>, decltype(mergeFiles1)> pool(int(ids.size()), mergeFiles1);
  for (auto& ids1 : ids) {
    auto& t = pool.waitFree();
    t.setup() = ids1;
    t.start();
  }
  pool.terminate();
  std::cout << "Parallel passes of externalMergePar: " << timerp << "sec.\n";
  Timer timer;
  mergeFiles(output, idsLast);
  std::cout << "Last pass of externalMergePar: " << timer << "sec.\n";
  std::cout << "Intermediate files: " << idsLast.back() + 1 << "\n";
}
#endif

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
#ifdef USE_THREADS
  if (numPasses == 0 && nFiles > 3) {
    externalMergePar(output, nFiles, numThreads);
    return;
  }
#endif
  if (numPasses == 1 || numPasses >= nFiles)
    externalMerge(output, nFiles, 0);
  else
    externalMerge(output, nFiles, nFiles / numPasses + 1);
  
}
