#include "merge.hpp"
#include "file.hpp"
#include <vector>
#include <climits>
#include <iostream>

template<class Data>
class MinHeap
{
  template<class Data1 = Data>
  struct Node
  {
    Data1 data;
    int i;
    bool operator<(const Node& r) const
    {
      return data < r.data || data == r.data && i < r.i;
    }
  };

  Node<Data>* pheap = nullptr;
  int size = 0;

public:

  MinHeap(int sz)
  {
    pheap = new Node<Data>[sz];
    size = sz;
  }

  ~MinHeap()
  {
    delete pheap;
    pheap = nullptr;
  }

  Node<Data>& operator[](int i) { return pheap[i]; }

  static int left(int i) { return 2 * i + 1; }

  static int right(int i) { return 2 * i + 2; }

  void init()
  {
    for (int i = (size - 1) / 2; i >= 0; i--)
      heapify(i);
  }

  void heapify(const int i)
  {
    int smaller = i;
    const int l = left(i);
    if (l < size && pheap[l] < pheap[i])
      smaller = l;
    const int r = right(i);
    if (r < size && pheap[r] < pheap[smaller])
      smaller = r;
    if (smaller != i) {
      std::swap(pheap[i], pheap[smaller]);
      heapify(smaller);
    }
  }
};

typedef unsigned Data;
const Data maxData = UINT_MAX;

#define BUFFERED_READ

void mergeFiles(const std::string& output, const std::vector<int>& ids)
{
  {
#ifdef BUFFERED_READ
    std::vector<FileReadBuf<Data>> ins; // Buffered input files, sorted pieces.
#else
    std::vector<File> ins; // Input files, sorted pieces.
#endif
    ins.reserve(ids.size());
    MinHeap<Data> heap(int(ids.size()));
    for (int i = 0; i < ids.size(); i++) {
#ifdef BUFFERED_READ
      ins.emplace_back(std::to_string(ids[i]));
#else
      ins.emplace_back(std::to_string(ids[i]), "rb"s);
#endif
      if (ins.back().read(heap[i].data))
        heap[i].i = i;
      else {
        // Strange bad file with no elements.
        heap[i].data = maxData;
        heap[i].i = INT_MAX;
      }
    }
    heap.init();

    FileWriteBuf<Data> buf(output);

    for (int finished = 0; finished < ids.size(); ) {
      auto& top = heap[0];
      if (ins.size() <= top.i)
        break;
      buf.push_back(top.data);
      if (!ins[top.i].read(top.data)) {
        top.data = maxData;
        top.i = INT_MAX;
        finished++;
      }
      heap.heapify(0);
    }

    if (buf.wasResized())
      std::cout << "Warning: FileWriteBuf resized its buffer to: " << buf.wasResized() << "\n";
  }
  for (auto id : ids)
    std::remove(std::to_string(id).data());
}