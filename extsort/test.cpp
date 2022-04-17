#include "test.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

typedef unsigned Data;

void generateSortedFile(const std::string& name, size_t size)
{
  FileWriteBuf<Data> buf(name);
  for (size_t i = 0; i < size; i++) {
    buf.push_back(Data(i));
  }
  if (buf.wasResized())
    std::cout << "Warning: FileWriteBuf resized its buffer to: " << buf.wasResized() << "\n";
}

void generateFile1(const std::string& name, size_t size)
{
  FileWriteBuf<Data> buf(name);
  srand(unsigned(time(0)));
  for (size_t i = 0; i < size; i++) {
    Data x = (Data(rand()) << 16) + rand();
    buf.push_back(x);
  }
  if (buf.wasResized())
    std::cout << "Warning: FileWriteBuf resized its buffer to: " << buf.wasResized() << "\n";
}

template< class Data, class Compare>
bool isSortedStream(File& f, Compare comp)
{
  auto read = [&f](Data& e) { return f && 1 == f.read(e); };
  Data prev, next;
  if (!read(prev))
    return true;
  while (read(next)) {
    if (!comp(prev, next))
      return false;
    prev = next;
  }
  return true;
}

void doReferenceSort(const std::string& origName, const std::string& resName)
{
  File forig(origName, "rb"s);
  size_t fileSize = forig.size();
  try
  {
    size_t bufSize = fileSize / sizeof(Data);
    auto* buf = new Data[bufSize];
    forig.read(buf, bufSize);
    std::sort(buf, buf + bufSize);
    File out(resName, "wb"s);
    out.write(buf, bufSize);
  }
  catch (const std::bad_alloc&)
  {
    std::cerr << "Cannot get mem buf enough to sort full input file.\n";
  }
}

bool makeTest(const std::string& origName, const std::string& resName)
{
  File forig(origName, "rb"s);
  size_t fileSize = forig.size();
  try
  {
    size_t bufSize = fileSize / sizeof(Data);
    auto* buf = new Data[bufSize];
    forig.read(buf, bufSize);
    std::sort(buf, buf + bufSize);

    File fres(resName, "rb"s);
    if (fileSize != fres.size())
      return false;
    std::vector<Data> smallBuf(256);
    for (size_t i = 0; fres && i < bufSize;) {
      auto sz = fres.read(smallBuf);
      if (0 != std::memcmp(
        reinterpret_cast<const char*>(buf + i),
        reinterpret_cast<const char*>(smallBuf.data()),
        sz * sizeof(Data))) {
        delete buf;
        return false;
      }
      i += sz;
    }
    delete buf;
    return true;
  }
  catch (const std::bad_alloc&)
  {
    std::cerr << "Cannot get mem buf enough to sort full input file. Just check result is sorted.\n";
    File fres(resName, "rb"s);
    if (fileSize != fres.size())
      return false;
    return isSortedStream<Data>(fres, std::less_equal<Data>());
  }
  return false;
}
