#include "text/SentenceSplitter.h"

#include <algorithm>
#include <cctype>

#include "text/Utf8.h"

namespace {
bool hasMeaningfulContent(const std::vector<std::string>& codepoints, std::size_t begin, std::size_t end) {
    for (std::size_t i = begin; i < end; ++i) {
        const std::string& cp = codepoints[i];
        if (utf8::isWhitespace(cp)) {
            continue;
        }
        if (cp == "." || cp == "," || cp == "!" || cp == "?" || cp == ":" || cp == ";" || cp == "\"" ||
            cp == "'" || cp == "(" || cp == ")" || cp == "[" || cp == "]" || cp == "{" || cp == "}" ||
            cp == "-" || cp == u8"，" || cp == u8"。" || cp == u8"！" || cp == u8"？" || cp == u8"：" ||
            cp == u8"；" || cp == u8"、" || cp == u8"“" || cp == u8"”" || cp == u8"‘" || cp == u8"’") {
            continue;
        }
        return true;
    }
    return false;
}

bool isAsciiWordChar(const std::string& cp) {
    if (cp.size() != 1) {
        return false;
    }

    const unsigned char ch = static_cast<unsigned char>(cp[0]);
    return std::isalnum(ch) != 0 || ch == '\'' || ch == '_';
}

std::size_t findWordBoundaryBefore(
    const std::vector<std::string>& codepoints,
    std::size_t lineStart,
    std::size_t overflowIndex) {
    if (overflowIndex <= lineStart || overflowIndex >= codepoints.size() || !isAsciiWordChar(codepoints[overflowIndex])) {
        return std::string::npos;
    }

    std::size_t wordStart = overflowIndex;
    while (wordStart > lineStart && isAsciiWordChar(codepoints[wordStart - 1])) {
        --wordStart;
    }

    if (wordStart > lineStart) {
        return wordStart - 1;
    }

    return std::string::npos;
}

bool isAsciiDigit(const std::string& cp) {
    return cp.size() == 1 && cp[0] >= '0' && cp[0] <= '9';
}
}

SentenceSplitter::SentenceSplitter(SplitOptions options) : options_(options) {
}

void SentenceSplitter::splitBlocks(
    const std::vector<TextBlock>& blocks,
    std::uint32_t chapterIndex,
    std::vector<Sentence>& outSentences) const {
    std::uint32_t paragraphIndex = 0;
    std::uint32_t sentenceIndex = 0;

    for (const TextBlock& block : blocks) {
        if (block.text.empty()) {
            ++paragraphIndex;
            continue;
        }

        if (block.type == BlockType::Title) {
            outSentences.push_back(Sentence{block.text, chapterIndex, paragraphIndex, sentenceIndex++, true});
            ++paragraphIndex;
            continue;
        }

        for (const std::string& text : splitParagraph(block.text)) {
            if (!text.empty()) {
                outSentences.push_back(Sentence{text, chapterIndex, paragraphIndex, sentenceIndex++, false});
            }
        }

        ++paragraphIndex;
    }
}

std::vector<std::string> SentenceSplitter::splitParagraph(const std::string& text) const {
    std::vector<std::string> result;
    const auto codepoints = utf8::splitCodepoints(text);
    std::size_t segmentStart = 0;
    std::size_t segmentLength = 0;
    std::size_t lastSoftBreak = std::string::npos;
    int quoteDepth = 0;

    auto flushRange = [&](std::size_t begin, std::size_t end) {
        while (begin < end && utf8::isWhitespace(codepoints[begin])) {
            ++begin;
        }
        while (end > begin && utf8::isWhitespace(codepoints[end - 1])) {
            --end;
        }
        if (begin < end && hasMeaningfulContent(codepoints, begin, end)) {
            result.push_back(utf8::join(codepoints, begin, end));
        }
    };

    auto isDecimalPointAt = [&](std::size_t index) {
        if (index == 0 || index + 1 >= codepoints.size() || codepoints[index] != ".") {
            return false;
        }
        return isAsciiDigit(codepoints[index - 1]) && isAsciiDigit(codepoints[index + 1]);
    };

    auto isNumericSeparatorAt = [&](std::size_t index) {
        if (index == 0 || index + 1 >= codepoints.size()) {
            return false;
        }
        const std::string& cp = codepoints[index];
        if (cp != "." && cp != ",") {
            return false;
        }
        return isAsciiDigit(codepoints[index - 1]) && isAsciiDigit(codepoints[index + 1]);
    };

    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        const std::string& cp = codepoints[i];
        ++segmentLength;

        if (cp == "\"" || cp == "'" || cp == "“" || cp == "「" || cp == "『" || cp == "‘") {
            ++quoteDepth;
        } else if ((cp == "\"" || cp == "'" || cp == "”" || cp == "」" || cp == "』" || cp == "’") &&
                   quoteDepth > 0) {
            --quoteDepth;
        }

        if (!isNumericSeparatorAt(i) && isSoftBreak(cp)) {
            lastSoftBreak = i;
        }

        if (!isDecimalPointAt(i) && !isNumericSeparatorAt(i) && isStrongBreak(cp) && quoteDepth == 0) {
            std::size_t end = i + 1;
            while (end < codepoints.size() && isClosingPunctuation(codepoints[end])) {
                ++end;
            }
            flushRange(segmentStart, end);
            segmentStart = end;
            segmentLength = 0;
            lastSoftBreak = std::string::npos;
            continue;
        }

        if (segmentLength >= options_.preferredMaxChars && lastSoftBreak != std::string::npos) {
            flushRange(segmentStart, lastSoftBreak + 1);
            segmentStart = lastSoftBreak + 1;
            while (segmentStart < codepoints.size() && utf8::isWhitespace(codepoints[segmentStart])) {
                ++segmentStart;
            }
            i = segmentStart > 0 ? segmentStart - 1 : 0;
            segmentLength = 0;
            lastSoftBreak = std::string::npos;
            continue;
        }

        if (segmentLength >= options_.hardMaxChars) {
            std::size_t hardBreak = findWordBoundaryBefore(codepoints, segmentStart, i);
            if (hardBreak != std::string::npos && hardBreak >= segmentStart) {
                flushRange(segmentStart, hardBreak + 1);
                segmentStart = hardBreak + 1;
                while (segmentStart < codepoints.size() && utf8::isWhitespace(codepoints[segmentStart])) {
                    ++segmentStart;
                }
                i = segmentStart > 0 ? segmentStart - 1 : 0;
            } else {
                flushRange(segmentStart, i + 1);
                segmentStart = i + 1;
            }
            segmentLength = 0;
            lastSoftBreak = std::string::npos;
        }
    }

    flushRange(segmentStart, codepoints.size());

    return result;
}

bool SentenceSplitter::isStrongBreak(const std::string& codepoint) {
    return codepoint == "." || codepoint == "!" || codepoint == "?" || codepoint == ";" ||
           codepoint == "。" || codepoint == "！" || codepoint == "？" || codepoint == "；" ||
           codepoint == "…" || codepoint == "……";
}

bool SentenceSplitter::isSoftBreak(const std::string& codepoint) {
    return codepoint == "," || codepoint == ":" || codepoint == "，" || codepoint == "：" ||
           codepoint == "、";
}

bool SentenceSplitter::isClosingPunctuation(const std::string& codepoint) {
    return codepoint == "\"" || codepoint == "'" || codepoint == "”" || codepoint == "’" ||
           codepoint == "」" || codepoint == "』" || codepoint == ")" || codepoint == "）";
}
