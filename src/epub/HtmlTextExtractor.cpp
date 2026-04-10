#include "epub/HtmlTextExtractor.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "text/Utf8.h"

namespace {
struct TagSpec {
    const char* tagName = "";
    BlockType type = BlockType::Paragraph;
};

std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

std::size_t findTagStart(const std::string& lowerHtml, const std::string& tagName, std::size_t from) {
    const std::string needle = "<" + tagName;
    return lowerHtml.find(needle, from);
}

std::size_t findTagOpenEnd(const std::string& html, std::size_t tagStart) {
    return html.find('>', tagStart);
}

std::size_t findTagCloseStart(const std::string& lowerHtml, const std::string& tagName, std::size_t from) {
    const std::string needle = "</" + tagName;
    return lowerHtml.find(needle, from);
}

void replaceAll(std::string& text, const std::string& from, const std::string& to) {
    std::string::size_type pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}
}  // namespace

bool HtmlTextExtractor::extract(const std::string& html, std::vector<TextBlock>& outBlocks) const {
    outBlocks.clear();

    const std::vector<TagSpec> tags{
        {"h1", BlockType::Title},
        {"h2", BlockType::Title},
        {"h3", BlockType::Title},
        {"blockquote", BlockType::Quote},
        {"li", BlockType::ListItem},
        {"p", BlockType::Paragraph},
    };

    const std::string lowerHtml = toLowerCopy(html);
    std::size_t cursor = 0;

    while (cursor < lowerHtml.size()) {
        std::size_t bestPos = std::string::npos;
        const TagSpec* bestTag = nullptr;

        for (const TagSpec& tag : tags) {
            const std::size_t pos = findTagStart(lowerHtml, tag.tagName, cursor);
            if (pos != std::string::npos && (bestPos == std::string::npos || pos < bestPos)) {
                bestPos = pos;
                bestTag = &tag;
            }
        }

        if (bestTag == nullptr) {
            break;
        }

        const std::size_t openEnd = findTagOpenEnd(html, bestPos);
        if (openEnd == std::string::npos) {
            break;
        }

        const std::size_t closeStart = findTagCloseStart(lowerHtml, bestTag->tagName, openEnd + 1);
        if (closeStart == std::string::npos) {
            cursor = openEnd + 1;
            continue;
        }

        std::string text = trim(decodeEntities(stripTags(html.substr(openEnd + 1, closeStart - openEnd - 1))));
        if (!text.empty()) {
            outBlocks.push_back(TextBlock{bestTag->type, text});
        }

        const std::size_t closeEnd = html.find('>', closeStart);
        cursor = closeEnd == std::string::npos ? closeStart + 1 : closeEnd + 1;
    }

    const std::vector<std::string> looseParagraphs = splitLooseParagraphs(extractBody(html));
    for (const std::string& paragraph : looseParagraphs) {
        const auto duplicate = std::find_if(outBlocks.begin(), outBlocks.end(), [&](const TextBlock& block) {
            return block.text == paragraph;
        });
        if (duplicate != outBlocks.end()) {
            continue;
        }
        outBlocks.push_back(TextBlock{BlockType::Paragraph, paragraph});
    }

    return !outBlocks.empty();
}

std::string HtmlTextExtractor::extractBody(const std::string& html) {
    const std::string lowerHtml = toLowerCopy(html);
    const std::size_t bodyStart = lowerHtml.find("<body");
    if (bodyStart == std::string::npos) {
        return html;
    }

    const std::size_t openEnd = html.find('>', bodyStart);
    if (openEnd == std::string::npos) {
        return html;
    }

    const std::size_t bodyEnd = lowerHtml.find("</body>", openEnd + 1);
    if (bodyEnd == std::string::npos) {
        return html.substr(openEnd + 1);
    }

    return html.substr(openEnd + 1, bodyEnd - openEnd - 1);
}

std::vector<std::string> HtmlTextExtractor::splitLooseParagraphs(const std::string& html) {
    std::string normalized = html;
    replaceAll(normalized, "\r\n", "\n");
    replaceAll(normalized, "\r", "\n");

    static const std::regex breakTagPattern(
        R"(<\s*(/)?\s*(p|div|blockquote|li|br|tr|section|article)\b[^>]*>)",
        std::regex::icase);
    normalized = std::regex_replace(normalized, breakTagPattern, "\n\n");

    static const std::regex headingPattern(
        R"(<\s*/?\s*h[1-6]\b[^>]*>)",
        std::regex::icase);
    normalized = std::regex_replace(normalized, headingPattern, "\n\n");

    normalized = decodeEntities(stripTags(normalized));

    static const std::regex blankLinesPattern(R"(\n\s*\n+)");
    normalized = std::regex_replace(normalized, blankLinesPattern, "\n\n");

    std::vector<std::string> paragraphs;
    std::stringstream stream(normalized);
    std::string chunk;
    std::string current;
    while (std::getline(stream, chunk, '\n')) {
        const std::string text = trim(chunk);
        if (text.empty()) {
            if (!current.empty()) {
                paragraphs.push_back(current);
                current.clear();
            }
            continue;
        }

        if (!current.empty()) {
            current += ' ';
        }
        current += text;
    }

    if (!current.empty()) {
        paragraphs.push_back(current);
    }

    std::vector<std::string> filtered;
    filtered.reserve(paragraphs.size());
    for (const std::string& paragraph : paragraphs) {
        if (!paragraph.empty()) {
            filtered.push_back(paragraph);
        }
    }
    return filtered;
}

std::string HtmlTextExtractor::decodeEntities(std::string text) {
    const std::pair<const char*, const char*> entities[] = {
        {"&nbsp;", " "},
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&#39;", "'"},
    };

    for (const auto& [from, to] : entities) {
        std::string::size_type pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos) {
            text.replace(pos, std::char_traits<char>::length(from), to);
            pos += std::char_traits<char>::length(to);
        }
    }

    return text;
}

std::string HtmlTextExtractor::stripTags(const std::string& html) {
    std::string out;
    out.reserve(html.size());

    bool insideTag = false;
    for (char ch : html) {
        if (ch == '<') {
            insideTag = true;
            out.push_back(' ');
            continue;
        }
        if (ch == '>') {
            insideTag = false;
            continue;
        }
        if (!insideTag) {
            out.push_back(ch);
        }
    }

    return out;
}

std::string HtmlTextExtractor::trim(const std::string& text) {
    const auto codepoints = utf8::splitCodepoints(text);
    std::size_t begin = 0;
    std::size_t end = codepoints.size();
    while (begin < end && utf8::isWhitespace(codepoints[begin])) {
        ++begin;
    }
    while (end > begin && utf8::isWhitespace(codepoints[end - 1])) {
        --end;
    }
    if (begin >= end) {
        return {};
    }
    return utf8::join(codepoints, begin, end);
}
