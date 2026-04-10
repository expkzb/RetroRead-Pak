#include "platform/sdl/SDLFileSystem.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string readEnvPath(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0' ? std::string(value) : fallback;
}
}

bool SDLFileSystem::initialize() {
    rootPath_ = readEnvPath("NEXTREADING_HOME", ".");
    booksPath_ = readEnvPath("NEXTREADING_BOOKS_PATH", rootPath_ + "/Books");
    cachePath_ = readEnvPath("NEXTREADING_CACHE_PATH", rootPath_ + "/BooksCache");
    savesPath_ = readEnvPath("NEXTREADING_SAVES_PATH", rootPath_ + "/Saves/RetroRead");
    assetsPath_ = readEnvPath("NEXTREADING_ASSETS_PATH", rootPath_ + "/assets");

    createDirectories(booksPath_);
    createDirectories(cachePath_);
    createDirectories(savesPath_);
    return true;
}

std::string SDLFileSystem::booksPath() const {
    return booksPath_;
}

std::string SDLFileSystem::cachePath() const {
    return cachePath_;
}

std::string SDLFileSystem::savesPath() const {
    return savesPath_;
}

std::string SDLFileSystem::assetsPath() const {
    return assetsPath_;
}

bool SDLFileSystem::fileExists(const std::string& path) const {
    return fs::exists(path);
}

bool SDLFileSystem::createDirectories(const std::string& path) {
    std::error_code ec;
    return fs::create_directories(path, ec) || fs::exists(path);
}

bool SDLFileSystem::readTextFile(const std::string& path, std::string& out) const {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    out = buffer.str();
    return true;
}

bool SDLFileSystem::writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    stream << content;
    return true;
}

std::vector<std::string> SDLFileSystem::listFiles(
    const std::string& path,
    const std::string& extension) const {
    std::vector<std::string> files;
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return files;
    }

    const std::string expectedExt = lowercase(extension);
    for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (lowercase(entry.path().extension().string()) == expectedExt) {
            files.push_back(entry.path().string());
        }
    }

    return files;
}

std::uint64_t SDLFileSystem::fileSize(const std::string& path) const {
    std::error_code ec;
    return fs::exists(path, ec) ? fs::file_size(path, ec) : 0;
}

std::uint64_t SDLFileSystem::modifiedTime(const std::string& path) const {
    std::error_code ec;
    const auto time = fs::last_write_time(path, ec);
    if (ec) {
        return 0;
    }

    return static_cast<std::uint64_t>(time.time_since_epoch().count());
}
