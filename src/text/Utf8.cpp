#include "text/Utf8.h"

#include <cstddef>

namespace utf8 {

std::vector<std::string> splitCodepoints(const std::string& text) {
    std::vector<std::string> result;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        std::size_t length = 1;
        if ((ch & 0x80) == 0x00) {
            length = 1;
        } else if ((ch & 0xE0) == 0xC0) {
            length = 2;
        } else if ((ch & 0xF0) == 0xE0) {
            length = 3;
        } else if ((ch & 0xF8) == 0xF0) {
            length = 4;
        }

        if (i + length > text.size()) {
            length = 1;
        }

        result.push_back(text.substr(i, length));
        i += length;
    }

    return result;
}

std::string join(const std::vector<std::string>& parts, std::size_t begin, std::size_t end) {
    std::string out;
    for (std::size_t i = begin; i < end && i < parts.size(); ++i) {
        out += parts[i];
    }
    return out;
}

bool isWhitespace(const std::string& codepoint) {
    return codepoint == " " || codepoint == "\t" || codepoint == "\n" || codepoint == "\r" ||
           codepoint == u8"\u00A0" || codepoint == u8"\u3000";
}

}  // namespace utf8
