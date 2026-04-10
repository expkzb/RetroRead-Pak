#pragma once

#include <string>
#include <vector>

namespace utf8 {

std::vector<std::string> splitCodepoints(const std::string& text);
std::string join(const std::vector<std::string>& parts, std::size_t begin, std::size_t end);
bool isWhitespace(const std::string& codepoint);

}  // namespace utf8
