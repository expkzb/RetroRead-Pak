#include "platform/nextui/NextUIFileSystem.h"

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
}

bool NextUIFileSystem::initialize() {
    createDirectories(booksPath());
    createDirectories(cachePath());
    createDirectories(savesPath());
    return true;
}

std::string NextUIFileSystem::booksPath() const {
    return resolveEnvPath("NEXTREADING_BOOKS_PATH", rootPath() + "/Books");
}

std::string NextUIFileSystem::cachePath() const {
    return resolveEnvPath("NEXTREADING_CACHE_PATH", rootPath() + "/BooksCache");
}

std::string NextUIFileSystem::savesPath() const {
    return resolveEnvPath("NEXTREADING_SAVES_PATH", rootPath() + "/Saves/RetroRead");
}

std::string NextUIFileSystem::assetsPath() const {
    return resolveEnvPath("NEXTREADING_ASSETS_PATH", rootPath() + "/pak/assets");
}

bool NextUIFileSystem::fileExists(const std::string& path) const {
    return fs::exists(path);
}

bool NextUIFileSystem::createDirectories(const std::string& path) {
    std::error_code ec;
    return fs::create_directories(path, ec) || fs::exists(path);
}

bool NextUIFileSystem::readTextFile(const std::string& path, std::string& out) const {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    out = buffer.str();
    return true;
}

bool NextUIFileSystem::writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    stream << content;
    return true;
}

std::vector<std::string> NextUIFileSystem::listFiles(
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

std::uint64_t NextUIFileSystem::fileSize(const std::string& path) const {
    std::error_code ec;
    return fs::exists(path, ec) ? fs::file_size(path, ec) : 0;
}

std::uint64_t NextUIFileSystem::modifiedTime(const std::string& path) const {
    std::error_code ec;
    const auto time = fs::last_write_time(path, ec);
    if (ec) {
        return 0;
    }

    return static_cast<std::uint64_t>(time.time_since_epoch().count());
}

std::string NextUIFileSystem::resolveEnvPath(
    const char* name,
    const std::string& fallback) const {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    return value;
}

std::string NextUIFileSystem::rootPath() const {
    const char* home = std::getenv("NEXTREADING_HOME");
    if (home != nullptr && *home != '\0') {
        return home;
    }

    const char* pak = std::getenv("NEXTUI_PAK_ROOT");
    if (pak != nullptr && *pak != '\0') {
        return pak;
    }

    const char* dataRoot = std::getenv("NEXTUI_DATA_ROOT");
    if (dataRoot != nullptr && *dataRoot != '\0') {
        return std::string(dataRoot) + "/RetroRead";
    }

    return "./RetroRead";
}
