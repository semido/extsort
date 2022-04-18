#pragma once
#include "file.hpp"

void generateSortedFile(const std::string& name, size_t size);

void generateFile1(const std::string& name, size_t size);

void doReferenceSort(const std::string& origName, const std::string& resName);

bool makeTest(const std::string& origName, const std::string& resName);
