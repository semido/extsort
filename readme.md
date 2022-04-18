### External sort demo project

#### Goal

Executable shall sort binary input file of 32bit ints and produce another file with sorted data in the same directory.
Available program memory is limited and supposed to be lass than input file size.
The project can only use standard library and c++14. It should be cross-platform.
By default mem limit is 128M, input file is 'input' located in the dir of executable, result is 'output'.

#### Build

1. To configure with cmake do one of the following:
   * cmake -S . -B ./build/win -G "Visual Studio 16 2019"
   * cmake -S . -B ./build/rel -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
   * cmake -S . -B ./build/deb -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug

2. Then build with:
   * cmake --build ./build/${config}
   * or use ALL_BUILD.vcxproj

#### Executable

1. Executable 'extsort' is in ./bin dir. This may slowdown the execution significantly if you are on WSL.
2. It expects input data file in the same dir.
3. It work in the dir of input file and produces output file there.
4. Options:
   * --gen1g : generate 1G file with 256M elements;
   * --gen : generate 1K file with 256 elements;
   * --sorted : generate sorted file;
   * --test : check results of sorting;
   * --ref : do reference in-memory sort, no check allowed;
   * -m N : limit the memory with number of elements;
   * -t N : limit number of available of threads;
   * -p N : define number of merge passes;
   * -p 0 : start external sort with multithread merge; this is default mode now;
   * -s N : define number of merge slots, i.e. how many files are opened for merge.

