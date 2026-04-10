#pragma once

#include "platform/FileSystem.h"

class SDLFileSystem final : public FileSystem {
public:
    bool initialize() override;

    std::string booksPath() const override;
    std::string cachePath() const override;
    std::string savesPath() const override;
    std::string assetsPath() const override;

    bool fileExists(const std::string& path) const override;
    bool createDirectories(const std::string& path) override;
    bool readTextFile(const std::string& path, std::string& out) const override;
    bool writeTextFile(const std::string& path, const std::string& content) override;
    std::vector<std::string> listFiles(
        const std::string& path,
        const std::string& extension) const override;
    std::uint64_t fileSize(const std::string& path) const override;
    std::uint64_t modifiedTime(const std::string& path) const override;

private:
    std::string rootPath_ = ".";
    std::string booksPath_;
    std::string cachePath_;
    std::string savesPath_;
    std::string assetsPath_;
};
