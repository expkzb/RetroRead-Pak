#pragma once

#include <string>
#include <vector>

#include "core/BookTypes.h"

class FileSystem;

class BookLibrary {
public:
    bool scan(FileSystem& fileSystem);
    const std::vector<BookListItem>& books() const;

private:
    static std::string titleFromPath(const std::string& path);

    std::vector<BookListItem> books_;
};
