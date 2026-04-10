#pragma once

#include <string>
#include <unordered_map>

#include "core/BookTypes.h"

class FileSystem;

class ProgressStore {
public:
    bool load(FileSystem& fileSystem);
    bool save(FileSystem& fileSystem) const;

    bool get(const std::string& bookId, ReadingProgress& out) const;
    void put(const ReadingProgress& progress);

private:
    std::unordered_map<std::string, ReadingProgress> progressByBook_;
};
