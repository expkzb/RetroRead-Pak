#pragma once

#include <string>
#include <vector>

#include "epub/TextBlock.h"

class HtmlTextExtractor {
public:
    bool extract(const std::string& html, std::vector<TextBlock>& outBlocks) const;

private:
    static std::string extractBody(const std::string& html);
    static std::vector<std::string> splitLooseParagraphs(const std::string& html);
    static std::string decodeEntities(std::string text);
    static std::string stripTags(const std::string& html);
    static std::string trim(const std::string& text);
};
