#include "core/BookCache.h"

#include <filesystem>
#include <sstream>
#include <string>

#include "platform/FileSystem.h"

namespace {
constexpr const char* kCacheVersion = "5";

std::string escapeField(std::string value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '\\' || ch == '\t' || ch == '\n') {
            out.push_back('\\');
            switch (ch) {
            case '\t':
                out.push_back('t');
                break;
            case '\n':
                out.push_back('n');
                break;
            default:
                out.push_back(ch);
                break;
            }
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::string unescapeField(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    bool escaping = false;
    for (char ch : value) {
        if (escaping) {
            switch (ch) {
            case 't':
                out.push_back('\t');
                break;
            case 'n':
                out.push_back('\n');
                break;
            default:
                out.push_back(ch);
                break;
            }
            escaping = false;
        } else if (ch == '\\') {
            escaping = true;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}
}

bool BookCache::load(FileSystem& fileSystem, const std::string& epubPath, BookScript& outBook) const {
    const std::string bookId = std::filesystem::path(epubPath).stem().string();
    std::string content;
    if (!fileSystem.readTextFile(cachePath(fileSystem, bookId), content)) {
        return false;
    }

    std::stringstream stream(content);
    std::string line;
    BookScript book;
    Chapter* currentChapter = nullptr;
    bool hasHeader = false;
    bool versionOk = false;
    std::uint64_t cachedSourceTime = 0;
    const std::uint64_t sourceTime = fileSystem.modifiedTime(epubPath);

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream lineStream(line);
        std::string kind;
        std::getline(lineStream, kind, '\t');

        if (kind == "CACHE") {
            std::string version;
            std::string modifiedTime;
            std::getline(lineStream, version, '\t');
            std::getline(lineStream, modifiedTime, '\t');
            hasHeader = true;
            versionOk = version == kCacheVersion;
            if (!modifiedTime.empty()) {
                cachedSourceTime = static_cast<std::uint64_t>(std::stoull(modifiedTime));
            }
        } else if (kind == "BOOK") {
            std::getline(lineStream, book.bookId, '\t');
            std::getline(lineStream, book.title, '\t');
            std::getline(lineStream, book.author, '\t');
            std::getline(lineStream, book.sourcePath, '\t');
            book.bookId = unescapeField(book.bookId);
            book.title = unescapeField(book.title);
            book.author = unescapeField(book.author);
            book.sourcePath = unescapeField(book.sourcePath);
        } else if (kind == "CHAPTER") {
            Chapter chapter;
            std::getline(lineStream, chapter.id, '\t');
            std::getline(lineStream, chapter.title, '\t');
            chapter.id = unescapeField(chapter.id);
            chapter.title = unescapeField(chapter.title);
            book.chapters.push_back(std::move(chapter));
            currentChapter = &book.chapters.back();
        } else if (kind == "SENTENCE" && currentChapter != nullptr) {
            std::string text;
            std::string isTitle;
            std::getline(lineStream, text, '\t');
            std::getline(lineStream, isTitle, '\t');
            currentChapter->sentences.push_back(Sentence{
                unescapeField(text),
                static_cast<std::uint32_t>(book.chapters.size() - 1),
                0,
                static_cast<std::uint32_t>(currentChapter->sentences.size()),
                isTitle == "1"});
        }
    }

    if (!hasHeader || !versionOk || (sourceTime != 0 && cachedSourceTime != sourceTime) || book.chapters.empty()) {
        return false;
    }

    outBook = std::move(book);
    return true;
}

bool BookCache::exists(FileSystem& fileSystem, const std::string& epubPath) const {
    const std::string bookId = std::filesystem::path(epubPath).stem().string();
    return fileSystem.fileExists(cachePath(fileSystem, bookId));
}

bool BookCache::save(FileSystem& fileSystem, const BookScript& book) const {
    fileSystem.createDirectories(fileSystem.cachePath());

    std::ostringstream out;
    out << "CACHE\t" << kCacheVersion
        << '\t' << fileSystem.modifiedTime(book.sourcePath)
        << '\n';
    out << "BOOK\t" << escapeField(book.bookId)
        << '\t' << escapeField(book.title)
        << '\t' << escapeField(book.author)
        << '\t' << escapeField(book.sourcePath)
        << '\n';

    for (const Chapter& chapter : book.chapters) {
        out << "CHAPTER\t" << escapeField(chapter.id)
            << '\t' << escapeField(chapter.title)
            << '\n';

        for (const Sentence& sentence : chapter.sentences) {
            out << "SENTENCE\t" << escapeField(sentence.text)
                << '\t' << (sentence.isTitle ? 1 : 0)
                << '\n';
        }
    }

    return fileSystem.writeTextFile(cachePath(fileSystem, book.bookId), out.str());
}

std::string BookCache::cachePath(FileSystem& fileSystem, const std::string& bookId) {
    return fileSystem.cachePath() + "/" + bookId + ".bookcache";
}
