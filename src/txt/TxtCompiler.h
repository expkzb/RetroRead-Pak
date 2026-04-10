#pragma once

#include <string>
#include <vector>

#include "core/BookTypes.h"
#include "epub/TextBlock.h"
#include "text/SentenceSplitter.h"

class TxtCompiler {
public:
    bool readMetadata(const std::string& txtPath, BookScript& outBook) const;
    bool compile(const std::string& txtPath, BookScript& outBook) const;

private:
    static bool readTextFileUtf8(const std::string& path, std::string& outText);
    static std::string normalizeNewlines(std::string text);
    static std::string trim(const std::string& text);
    static std::string collapseSpaces(std::string text);
    static bool isLikelyChapterTitle(const std::string& line);
    static std::vector<TextBlock> parseBlocks(const std::string& text);

    SentenceSplitter splitter_;
};
