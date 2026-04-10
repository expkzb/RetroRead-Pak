#pragma once

#include <string>

#include "core/BookTypes.h"

class FileSystem;

class BookCache {
public:
    bool exists(FileSystem& fileSystem, const std::string& epubPath) const;
    bool load(FileSystem& fileSystem, const std::string& epubPath, BookScript& outBook) const;
    bool save(FileSystem& fileSystem, const BookScript& book) const;

private:
    static std::string cachePath(FileSystem& fileSystem, const std::string& bookId);
};
