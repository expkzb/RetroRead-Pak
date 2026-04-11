#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/BookTypes.h"
#include "epub/TextBlock.h"

struct SplitOptions {
    std::size_t preferredMaxChars = 48;
    std::size_t hardMaxChars = 72;
};

class SentenceSplitter {
public:
    explicit SentenceSplitter(SplitOptions options = {});

    void splitBlocks(
        const std::vector<TextBlock>& blocks,
        std::uint32_t chapterIndex,
        std::vector<Sentence>& outSentences) const;

private:
    std::vector<std::string> splitParagraph(const std::string& text) const;
    static bool isStrongBreak(const std::string& codepoint);
    static bool isSoftBreak(const std::string& codepoint);
    static bool isClosingPunctuation(const std::string& codepoint);

    SplitOptions options_;
};
