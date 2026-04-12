#pragma once

#include <string>
#include <vector>

#include "core/BookTypes.h"
#include "epub/EpubArchive.h"
#include "epub/HtmlTextExtractor.h"
#include "text/SentenceSplitter.h"

class EpubCompiler {
public:
    bool readMetadata(const std::string& epubPath, BookScript& outBook) const;
    bool compile(const std::string& epubPath, BookScript& outBook) const;
    bool compileChapterList(const std::string& epubPath, BookScript& outBook) const;

private:
    struct NavPoint {
        std::string title;
        std::string srcPath;
        std::string anchor;
    };

    static std::string parseRootFilePath(const std::string& containerXml);
    static std::string parseBetween(const std::string& text, const std::string& start, const std::string& end);
    static std::string parseTagAttribute(const std::string& tag, const std::string& attribute);
    static std::string joinOpfPath(const std::string& opfPath, const std::string& href);
    static std::vector<NavPoint> parseNavPoints(const std::string& ncxXml, const std::string& opfPath);
    static std::pair<std::string, std::string> splitHrefAnchor(const std::string& href);
    static std::string extractAnchoredSectionHtml(
        const std::string& html,
        const std::string& anchor,
        const std::string& nextAnchor);

    EpubArchive archive_;
    HtmlTextExtractor extractor_;
    SentenceSplitter splitter_;
};
