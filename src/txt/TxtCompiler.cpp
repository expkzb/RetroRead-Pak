#include "txt/TxtCompiler.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <utility>

namespace {
std::string utf8FromCodepoint(std::uint32_t codepoint) {
    std::string out;
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return out;
}

std::string utf8FromUtf16(const std::string& bytes, bool littleEndian) {
    std::string out;
    if (bytes.size() < 2) {
        return out;
    }

    for (std::size_t i = 0; i + 1 < bytes.size(); i += 2) {
        const unsigned char b0 = static_cast<unsigned char>(bytes[i]);
        const unsigned char b1 = static_cast<unsigned char>(bytes[i + 1]);
        std::uint16_t unit = littleEndian ? static_cast<std::uint16_t>(b0 | (b1 << 8))
                                          : static_cast<std::uint16_t>((b0 << 8) | b1);
        if (unit == 0xFEFF) {
            continue;
        }

        if (unit >= 0xD800 && unit <= 0xDBFF && i + 3 < bytes.size()) {
            const unsigned char b2 = static_cast<unsigned char>(bytes[i + 2]);
            const unsigned char b3 = static_cast<unsigned char>(bytes[i + 3]);
            const std::uint16_t low = littleEndian ? static_cast<std::uint16_t>(b2 | (b3 << 8))
                                                   : static_cast<std::uint16_t>((b2 << 8) | b3);
            if (low >= 0xDC00 && low <= 0xDFFF) {
                const std::uint32_t codepoint =
                    0x10000u + (((static_cast<std::uint32_t>(unit) - 0xD800u) << 10) |
                                (static_cast<std::uint32_t>(low) - 0xDC00u));
                out += utf8FromCodepoint(codepoint);
                i += 2;
                continue;
            }
        }

        out += utf8FromCodepoint(unit);
    }

    return out;
}
}

bool TxtCompiler::readMetadata(const std::string& txtPath, BookScript& outBook) const {
    outBook = {};
    outBook.sourcePath = txtPath;
    outBook.bookId = std::filesystem::path(txtPath).stem().string();
    outBook.title = outBook.bookId;
    return true;
}

bool TxtCompiler::compile(const std::string& txtPath, BookScript& outBook) const {
    if (!readMetadata(txtPath, outBook)) {
        return false;
    }

    std::string text;
    if (!readTextFileUtf8(txtPath, text)) {
        return false;
    }

    const std::vector<TextBlock> blocks = parseBlocks(normalizeNewlines(std::move(text)));
    if (blocks.empty()) {
        return false;
    }

    std::vector<TextBlock> currentBlocks;
    std::uint32_t chapterIndex = 0;

    auto flushChapter = [&](const std::string& title) {
        if (currentBlocks.empty()) {
            return;
        }

        Chapter chapter;
        chapter.id = "txt-" + std::to_string(chapterIndex + 1);
        chapter.title = title.empty() ? "Chapter " + std::to_string(chapterIndex + 1) : title;
        splitter_.splitBlocks(currentBlocks, chapterIndex, chapter.sentences);
        if (!chapter.sentences.empty()) {
            outBook.chapters.push_back(std::move(chapter));
            ++chapterIndex;
        }
        currentBlocks.clear();
    };

    std::string pendingTitle;
    for (const TextBlock& block : blocks) {
        if (block.type == BlockType::Title && !currentBlocks.empty()) {
            flushChapter(pendingTitle);
        }

        if (block.type == BlockType::Title) {
            pendingTitle = block.text;
        }

        currentBlocks.push_back(block);
    }

    flushChapter(pendingTitle);

    if (outBook.chapters.empty()) {
        Chapter chapter;
        chapter.id = "txt-1";
        chapter.title = outBook.title;
        splitter_.splitBlocks(blocks, 0, chapter.sentences);
        if (!chapter.sentences.empty()) {
            outBook.chapters.push_back(std::move(chapter));
        }
    }

    return !outBook.chapters.empty();
}

bool TxtCompiler::readTextFileUtf8(const std::string& path, std::string& outText) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    std::string bytes = buffer.str();
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        outText = bytes.substr(3);
        return true;
    }

    if (bytes.size() >= 2 &&
        static_cast<unsigned char>(bytes[0]) == 0xFF &&
        static_cast<unsigned char>(bytes[1]) == 0xFE) {
        outText = utf8FromUtf16(bytes.substr(2), true);
        return true;
    }

    if (bytes.size() >= 2 &&
        static_cast<unsigned char>(bytes[0]) == 0xFE &&
        static_cast<unsigned char>(bytes[1]) == 0xFF) {
        outText = utf8FromUtf16(bytes.substr(2), false);
        return true;
    }

    outText = std::move(bytes);
    return true;
}

std::string TxtCompiler::normalizeNewlines(std::string text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            out.push_back('\n');
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::string TxtCompiler::trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && static_cast<unsigned char>(text[begin]) <= 0x20) {
        ++begin;
    }
    while (end > begin && static_cast<unsigned char>(text[end - 1]) <= 0x20) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string TxtCompiler::collapseSpaces(std::string text) {
    std::string out;
    out.reserve(text.size());
    bool lastWasSpace = false;
    for (unsigned char ch : text) {
        if (ch == ' ' || ch == '\t') {
            if (!lastWasSpace) {
                out.push_back(' ');
            }
            lastWasSpace = true;
        } else {
            out.push_back(static_cast<char>(ch));
            lastWasSpace = false;
        }
    }
    return trim(out);
}

bool TxtCompiler::isLikelyChapterTitle(const std::string& line) {
    const std::string compact = collapseSpaces(line);
    if (compact.empty() || compact.size() > 80) {
        return false;
    }

    static const std::regex chinesePattern(
        u8R"(^(第\s*[0-9一二三四五六七八九十百千万零两〇IVXLCDMivxlcdm]+\s*[章节卷部篇回幕集]|序章|终章|后记|尾声|番外))");
    static const std::regex englishPattern(
        R"(^(chapter|part|book|prologue|epilogue)\b[\s\.:_-]*[0-9ivxlcdm]*)",
        std::regex::icase);

    return std::regex_search(compact, chinesePattern) || std::regex_search(compact, englishPattern);
}

std::vector<TextBlock> TxtCompiler::parseBlocks(const std::string& text) {
    std::vector<TextBlock> blocks;
    std::stringstream stream(text);
    std::string line;
    std::vector<std::string> paragraphLines;

    auto flushParagraph = [&]() {
        if (paragraphLines.empty()) {
            return;
        }

        std::string paragraph;
        for (std::size_t i = 0; i < paragraphLines.size(); ++i) {
            if (!paragraph.empty()) {
                paragraph.push_back(' ');
            }
            paragraph += collapseSpaces(paragraphLines[i]);
        }

        paragraph = trim(paragraph);
        if (!paragraph.empty()) {
            blocks.push_back(TextBlock{BlockType::Paragraph, paragraph});
        }
        paragraphLines.clear();
    };

    while (std::getline(stream, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            flushParagraph();
            continue;
        }

        if (isLikelyChapterTitle(trimmed)) {
            flushParagraph();
            blocks.push_back(TextBlock{BlockType::Title, collapseSpaces(trimmed)});
            continue;
        }

        paragraphLines.push_back(trimmed);
    }

    flushParagraph();
    return blocks;
}
