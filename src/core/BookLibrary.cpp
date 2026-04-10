#include "core/BookLibrary.h"

#include <filesystem>

#include "platform/FileSystem.h"

bool BookLibrary::scan(FileSystem& fileSystem) {
    books_.clear();
    auto files = fileSystem.listFiles(fileSystem.booksPath(), ".epub");
    const auto txtFiles = fileSystem.listFiles(fileSystem.booksPath(), ".txt");
    files.insert(files.end(), txtFiles.begin(), txtFiles.end());
    for (const std::string& file : files) {
        books_.push_back(BookListItem{file, titleFromPath(file)});
    }
    return true;
}

const std::vector<BookListItem>& BookLibrary::books() const {
    return books_;
}

std::string BookLibrary::titleFromPath(const std::string& path) {
    return std::filesystem::path(path).stem().string();
}
