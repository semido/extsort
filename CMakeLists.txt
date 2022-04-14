cmake_minimum_required(VERSION 3.18)

# External sort demo project.
project(extsort CXX)

# Put all targets in the same directory rather than per project as by default.
include(GNUInstallDirs)
set(stageDir ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_INSTALL_BINDIR})
message("stageDir=${stageDir}")
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${stageDir})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${stageDir})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${stageDir})
endif()

# Put sources into appropriate subfolders in vcxproj-es.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
  set(CMAKE_CXX_STANDARD 14)
#  add_compile_options("-Zc:__cplusplus")
else()
  add_compile_options("-std=c++14")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -pthread")
endif()

add_executable(extsort main.cpp)

target_sources(extsort PRIVATE readme.md)

add_subdirectory(extsort)
