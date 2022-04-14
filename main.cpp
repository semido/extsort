#include "extsort/cmd.hpp"
#include "extsort/extsort.hpp"
#include "extsort/test.hpp"
#include <thread>
#include <iostream>
#include <string>

int main(int argc, const char* argv[])
{
#ifdef _DEBUG
  __debugbreak();
#endif
  std::setlocale(LC_ALL, ".UTF-8");
  std::cout << "Hi, балбесик 😊\n";

  CmdOptions cmd(argc, argv);

  const std::string testName = "input";
  if (cmd.exists_option("--gen1g") || cmd.exists_option("--gen"))
  {
    Timer timer;
    size_t sz = cmd.exists_option("--gen1g") ? 256 * 1024 * 1024UL : 256;
    std::string sorted;
    if (cmd.exists_option("--sorted")) {
      sorted = " Sorted.";
      generateSortedFile(testName, sz);
    }
    else
      generateFile1(testName, sz);
    std::cout << "Generate test file: " << timer << "sec. Size:" << sz << sorted << "\n";
  }

  const std::string resultName = "output";
  if (cmd.exists_option("--ref")) {
    Timer timer;
    doReferenceSort(testName, resultName);
    std::cout << "Make reference in-memory sort: " << timer << "sec\n";
    return 0; // No test needed.
  }

  {
    size_t memSize = 128 * 1024 * 1024UL;
    if (cmd.exists_option("-m"))
      memSize = std::stoi(cmd.get_option("-m")) * sizeof(unsigned);
    int numThreads = std::thread::hardware_concurrency();
    if (cmd.exists_option("-t"))
      numThreads = std::stoi(cmd.get_option("-t"));
    Timer timer;
    if (cmd.exists_option("-p")) {
      int numPasses = std::stoi(cmd.get_option("-p"));
      externalSortNPasses(testName, resultName, memSize, numThreads, numPasses);
    }
    else {
      int numSlots = std::stoi(cmd.get_option("-s", "0"));
      externalSort(testName, resultName, memSize, numThreads, numSlots);
    }
    std::cout << "External sort: " << timer << "sec\n";
  }

  if (cmd.exists_option("--test")) {
    Timer timer;
    auto test = makeTest(testName, resultName) ? "passed"s : "failed"s;
    std::cout << "Check results: " << timer << "sec\n";
    std::cout << "Test " << test << "\n";
  }
}
