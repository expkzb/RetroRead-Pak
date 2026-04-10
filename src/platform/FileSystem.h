#pragma once

#include <cstdint>
#include <string>
#include <vector>

class FileSystem {
public:
    virtual ~FileSystem() = default;

    virtual bool initialize() = 0;

    virtual std::string booksPath() const = 0;
    virtual std::string cachePath() const = 0;
    virtual std::string savesPath() const = 0;
    virtual std::string assetsPath() const = 0;

    virtual bool fileExists(const std::string& path) const = 0;
    virtual bool createDirectories(const std::string& path) = 0;
    virtual bool readTextFile(const std::string& path, std::string& out) const = 0;
    virtual bool writeTextFile(const std::string& path, const std::string& content) = 0;
    virtual std::vector<std::string> listFiles(
        const std::string& path,
        const std::string& extension) const = 0;
    virtual std::uint64_t fileSize(const std::string& path) const = 0;
    virtual std::uint64_t modifiedTime(const std::string& path) const = 0;
};
