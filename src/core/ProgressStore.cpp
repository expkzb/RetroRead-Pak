#include "core/ProgressStore.h"

#include <sstream>
#include <string>
#include <vector>

#include "platform/FileSystem.h"

namespace {
std::string progressPath(FileSystem& fileSystem) {
    return fileSystem.savesPath() + "/progress.db";
}

std::vector<std::string> splitTab(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream stream(line);
    std::string part;
    while (std::getline(stream, part, '\t')) {
        parts.push_back(part);
    }
    return parts;
}
}  // namespace

bool ProgressStore::load(FileSystem& fileSystem) {
    progressByBook_.clear();

    std::string content;
    if (!fileSystem.readTextFile(progressPath(fileSystem), content)) {
        return true;
    }

    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> parts = splitTab(line);
        if (parts.size() < 5) {
            continue;
        }

        ReadingProgress progress;
        progress.bookId = parts[0];
        progress.chapterIndex = static_cast<std::uint32_t>(std::stoul(parts[1]));
        progress.sentenceIndex = static_cast<std::uint32_t>(std::stoul(parts[2]));
        progress.autoPlay = parts[3] == "1";
        progress.textSpeed = static_cast<std::uint32_t>(std::stoul(parts[4]));
        if (parts.size() >= 6) {
            progress.lastOpenedAt = static_cast<std::uint64_t>(std::stoull(parts[5]));
        }
        progressByBook_[progress.bookId] = progress;
    }

    return true;
}

bool ProgressStore::save(FileSystem& fileSystem) const {
    fileSystem.createDirectories(fileSystem.savesPath());

    std::ostringstream out;
    for (const auto& [bookId, progress] : progressByBook_) {
        out << bookId << '\t'
            << progress.chapterIndex << '\t'
            << progress.sentenceIndex << '\t'
            << (progress.autoPlay ? 1 : 0) << '\t'
            << progress.textSpeed << '\t'
            << progress.lastOpenedAt << '\n';
    }

    return fileSystem.writeTextFile(progressPath(fileSystem), out.str());
}

bool ProgressStore::get(const std::string& bookId, ReadingProgress& out) const {
    const auto it = progressByBook_.find(bookId);
    if (it == progressByBook_.end()) {
        return false;
    }

    out = it->second;
    return true;
}

void ProgressStore::put(const ReadingProgress& progress) {
    progressByBook_[progress.bookId] = progress;
}
