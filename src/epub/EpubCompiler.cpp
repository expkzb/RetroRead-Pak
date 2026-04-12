#include "epub/EpubCompiler.h"

#include <algorithm>
#include <filesystem>
#include <numeric>
#include <optional>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "text/Utf8.h"

namespace {
std::string normalizeText(std::string text) {
    const std::string ideographicSpace = u8"\u3000";
    std::string::size_type pos = 0;
    while ((pos = text.find(ideographicSpace, pos)) != std::string::npos) {
        text.replace(pos, ideographicSpace.size(), " ");
        pos += 1;
    }

    auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    if (begin >= end) {
        return {};
    }

    return std::string(begin, end);
}

std::size_t codepointCount(const std::string& text) {
    return utf8::splitCodepoints(text).size();
}

bool isLikelySectionTitle(const std::string& title) {
    static const std::regex sectionPattern(u8R"(^(第[一二三四五六七八九十百零〇兩0-9]+[章幕節回卷部篇]|序章|終章|尾聲|後記|番外))");
    return std::regex_search(title, sectionPattern);
}

std::string removeSpaces(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch) != 0; }), text.end());
    const std::string ideographicSpace = u8"\u3000";
    std::string::size_type pos = 0;
    while ((pos = text.find(ideographicSpace, pos)) != std::string::npos) {
        text.erase(pos, ideographicSpace.size());
    }
    return text;
}

bool isShortOrdinalTitle(const std::string& title) {
    const std::string compact = removeSpaces(title);
    if (compact.empty() || codepointCount(compact) > 4) {
        return false;
    }

    const auto cps = utf8::splitCodepoints(compact);
    for (std::size_t i = 0; i < cps.size(); ++i) {
        const std::string& cp = cps[i];
        if (i == 0 && cp == u8"第") {
            continue;
        }

        if (cp != u8"一" && cp != u8"二" && cp != u8"三" && cp != u8"四" && cp != u8"五" &&
            cp != u8"六" && cp != u8"七" && cp != u8"八" && cp != u8"九" && cp != u8"十" &&
            cp != u8"百" && cp != u8"零" && cp != u8"〇" && cp != u8"兩" &&
            (cp.size() != 1 || cp[0] < '0' || cp[0] > '9')) {
            return false;
        }
    }

    return true;
}

bool shouldSkipChapter(const std::vector<TextBlock>& blocks, const std::vector<Sentence>& sentences) {
    std::size_t nonEmptyBlocks = 0;
    std::size_t totalChars = 0;

    for (const TextBlock& block : blocks) {
        const std::string text = normalizeText(block.text);
        if (text.empty()) {
            continue;
        }
        ++nonEmptyBlocks;
        totalChars += codepointCount(text);
    }

    if (nonEmptyBlocks == 0 || sentences.empty()) {
        return true;
    }

    return false;
}

std::string chooseChapterTitle(
    const std::vector<TextBlock>& blocks,
    const std::vector<Sentence>& sentences,
    const std::string& lastMajorTitle,
    std::uint32_t chapterNumber) {
    std::string title;
    for (const TextBlock& block : blocks) {
        if (block.type == BlockType::Title) {
            title = normalizeText(block.text);
            if (!title.empty()) {
                break;
            }
        }
    }

    if (title.empty() && !sentences.empty()) {
        const std::string first = normalizeText(sentences.front().text);
        const std::size_t totalChars = std::accumulate(
            sentences.begin(),
            sentences.end(),
            static_cast<std::size_t>(0),
            [](std::size_t sum, const Sentence& sentence) { return sum + codepointCount(sentence.text); });

        if (sentences.size() <= 6 && totalChars <= 80) {
            return chapterNumber == 1 ? u8"引文" : u8"短章";
        }

        return "Chapter " + std::to_string(chapterNumber);
    }

    if (isShortOrdinalTitle(title) && !lastMajorTitle.empty()) {
        return lastMajorTitle + " / " + title;
    }

    return title.empty() ? "Chapter " + std::to_string(chapterNumber) : title;
}

bool isUsefulNavTitle(const std::string& title) {
    const std::string normalized = normalizeText(title);
    return !normalized.empty();
}

bool isExplicitChapterMarker(const TextBlock& block) {
    const std::string text = normalizeText(block.text);
    if (text.empty()) {
        return false;
    }

    static const std::regex chinesePattern(u8R"(^(第\s*[一二三四五六七八九十百千万零〇两0-9\-—]+\s*章)$)");
    static const std::regex englishPattern(R"(^(Chapter\s+[0-9IVXLCDM]+)$)", std::regex::icase);
    return std::regex_match(text, chinesePattern) || std::regex_match(text, englishPattern);
}

bool isLikelyChapterSubtitle(const TextBlock& block) {
    const std::string text = normalizeText(block.text);
    if (text.empty()) {
        return false;
    }
    if (isExplicitChapterMarker(block)) {
        return false;
    }
    if (codepointCount(text) > 24) {
        return false;
    }
    return text.find_first_of(".!?。！？：:") == std::string::npos;
}

struct InlineChapterSlice {
    std::size_t startIndex = 0;
    std::size_t endIndex = 0;
    std::string title;
};

std::vector<InlineChapterSlice> splitInlineChapters(const std::vector<TextBlock>& blocks) {
    std::vector<InlineChapterSlice> slices;
    std::vector<std::size_t> starts;

    for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (isExplicitChapterMarker(blocks[i])) {
            starts.push_back(i);
        }
    }

    if (starts.empty()) {
        return slices;
    }

    if (starts.front() > 0) {
        std::size_t prefaceChars = 0;
        for (std::size_t i = 0; i < starts.front(); ++i) {
            prefaceChars += codepointCount(normalizeText(blocks[i].text));
        }
        if (prefaceChars >= 80) {
            std::string prefaceTitle = "Preface";
            for (std::size_t i = 0; i < starts.front(); ++i) {
                const std::string text = normalizeText(blocks[i].text);
                if (!text.empty() && !isExplicitChapterMarker(blocks[i])) {
                    prefaceTitle = text;
                    break;
                }
            }
            slices.push_back(InlineChapterSlice{0, starts.front(), prefaceTitle});
        }
    }

    for (std::size_t idx = 0; idx < starts.size(); ++idx) {
        const std::size_t start = starts[idx];
        const std::size_t end = idx + 1 < starts.size() ? starts[idx + 1] : blocks.size();
        std::string title = normalizeText(blocks[start].text);
        if (start + 1 < end && isLikelyChapterSubtitle(blocks[start + 1])) {
            title += " " + normalizeText(blocks[start + 1].text);
        }
        slices.push_back(InlineChapterSlice{start, end, title});
    }

    return slices;
}
}  // namespace

bool EpubCompiler::readMetadata(const std::string& epubPath, BookScript& outBook) const {
    EpubArchive archive;
    if (!archive.open(epubPath)) {
        return false;
    }

    std::string containerXml;
    if (!archive.readTextFile("META-INF/container.xml", containerXml)) {
        return false;
    }

    const std::string opfPath = parseRootFilePath(containerXml);
    if (opfPath.empty()) {
        return false;
    }

    std::string opfXml;
    if (!archive.readTextFile(opfPath, opfXml)) {
        return false;
    }

    outBook = {};
    outBook.sourcePath = epubPath;
    outBook.bookId = std::filesystem::path(epubPath).stem().string();
    outBook.title = parseBetween(opfXml, "<dc:title>", "</dc:title>");
    outBook.author = parseBetween(opfXml, "<dc:creator", "</dc:creator>");
    if (outBook.author.rfind(">", 0) == 0) {
        outBook.author.erase(0, 1);
    }
    if (outBook.title.empty()) {
        outBook.title = outBook.bookId;
    }

    return true;
}

bool EpubCompiler::compile(const std::string& epubPath, BookScript& outBook) const {
    if (!readMetadata(epubPath, outBook)) {
        return false;
    }

    EpubArchive archive;
    archive.open(epubPath);

    std::string containerXml;
    archive.readTextFile("META-INF/container.xml", containerXml);
    const std::string opfPath = parseRootFilePath(containerXml);

    std::string opfXml;
    archive.readTextFile(opfPath, opfXml);

    const std::regex manifestPattern("<item\\b[^>]*>", std::regex::icase);
    const std::regex spinePattern(
        "<itemref[^>]*idref\\s*=\\s*\"([^\"]+)\"",
        std::regex::icase);

    std::vector<std::tuple<std::string, std::string, std::string>> manifest;
    for (auto it = std::sregex_iterator(opfXml.begin(), opfXml.end(), manifestPattern);
         it != std::sregex_iterator();
         ++it) {
        const std::string tag = (*it)[0].str();
        const std::string id = parseTagAttribute(tag, "id");
        const std::string href = parseTagAttribute(tag, "href");
        const std::string mediaType = parseTagAttribute(tag, "media-type");
        if (!id.empty() && !href.empty() && !mediaType.empty()) {
            manifest.emplace_back(id, href, mediaType);
        }
    }

    std::vector<std::string> spineIds;
    for (auto it = std::sregex_iterator(opfXml.begin(), opfXml.end(), spinePattern);
         it != std::sregex_iterator();
         ++it) {
        spineIds.push_back((*it)[1].str());
    }

    std::string ncxPath;
    std::string spineTag;
    {
        const std::regex spineTagPattern(R"(<spine\b[^>]*>)", std::regex::icase);
        std::smatch spineMatch;
        if (std::regex_search(opfXml, spineMatch, spineTagPattern)) {
            spineTag = spineMatch[0].str();
        }
    }
    const std::string tocId = parseTagAttribute(spineTag, "toc");
    if (!tocId.empty()) {
        auto tocMatch = std::find_if(manifest.begin(), manifest.end(), [&](const auto& item) {
            return std::get<0>(item) == tocId;
        });
        if (tocMatch != manifest.end()) {
            ncxPath = joinOpfPath(opfPath, std::get<1>(*tocMatch));
        }
    }

    std::vector<NavPoint> navPoints;
    if (!ncxPath.empty()) {
        std::string ncxXml;
        if (archive.readTextFile(ncxPath, ncxXml)) {
            navPoints = parseNavPoints(ncxXml, opfPath);
        }
    }

    std::uint32_t chapterIndex = 0;
    std::string lastMajorTitle;
    for (const std::string& idref : spineIds) {
        auto match = std::find_if(manifest.begin(), manifest.end(), [&](const auto& item) {
            return std::get<0>(item) == idref;
        });

        if (match == manifest.end()) {
            continue;
        }

        const std::string& href = std::get<1>(*match);
        const std::string& mediaType = std::get<2>(*match);
        if (mediaType != "application/xhtml+xml" && mediaType != "text/html") {
            continue;
        }

        std::string html;
        if (!archive.readTextFile(joinOpfPath(opfPath, href), html)) {
            continue;
        }

        const std::string contentPath = joinOpfPath(opfPath, href);
        std::vector<NavPoint> fileNavPoints;
        for (const NavPoint& navPoint : navPoints) {
            if (navPoint.srcPath == contentPath && !navPoint.anchor.empty() && isUsefulNavTitle(navPoint.title)) {
                fileNavPoints.push_back(navPoint);
            }
        }

        if (fileNavPoints.size() >= 2) {
            for (std::size_t i = 0; i < fileNavPoints.size(); ++i) {
                const std::string nextAnchor = i + 1 < fileNavPoints.size() ? fileNavPoints[i + 1].anchor : "";
                const std::string sectionHtml =
                    extractAnchoredSectionHtml(html, fileNavPoints[i].anchor, nextAnchor);
                if (sectionHtml.empty()) {
                    continue;
                }

                std::vector<TextBlock> blocks;
                if (!extractor_.extract(sectionHtml, blocks)) {
                    continue;
                }

                Chapter chapter;
                chapter.id = idref + "#" + fileNavPoints[i].anchor;
                splitter_.splitBlocks(blocks, chapterIndex, chapter.sentences);
                if (shouldSkipChapter(blocks, chapter.sentences)) {
                    continue;
                }

                chapter.title = normalizeText(fileNavPoints[i].title);
                if (chapter.title.empty()) {
                    chapter.title = chooseChapterTitle(blocks, chapter.sentences, lastMajorTitle, chapterIndex + 1);
                }
                if (isLikelySectionTitle(chapter.title) && chapter.title.find(" / ") == std::string::npos) {
                    lastMajorTitle = chapter.title;
                }

                outBook.chapters.push_back(std::move(chapter));
                ++chapterIndex;
            }
            continue;
        }

        std::vector<TextBlock> blocks;
        if (!extractor_.extract(html, blocks)) {
            continue;
        }

        const std::vector<InlineChapterSlice> inlineSlices = splitInlineChapters(blocks);
        if (inlineSlices.size() >= 2) {
            for (const InlineChapterSlice& slice : inlineSlices) {
                if (slice.startIndex >= slice.endIndex || slice.endIndex > blocks.size()) {
                    continue;
                }

                std::vector<TextBlock> chapterBlocks(
                    blocks.begin() + static_cast<std::ptrdiff_t>(slice.startIndex),
                    blocks.begin() + static_cast<std::ptrdiff_t>(slice.endIndex));

                if (!chapterBlocks.empty()) {
                    chapterBlocks.front().type = BlockType::Title;
                    if (chapterBlocks.size() > 1 && isLikelyChapterSubtitle(chapterBlocks[1])) {
                        chapterBlocks[1].type = BlockType::Title;
                    }
                }

                Chapter chapter;
                chapter.id = idref + "-" + std::to_string(chapterIndex + 1);
                splitter_.splitBlocks(chapterBlocks, chapterIndex, chapter.sentences);
                if (shouldSkipChapter(chapterBlocks, chapter.sentences)) {
                    continue;
                }

                chapter.title = !slice.title.empty()
                    ? slice.title
                    : chooseChapterTitle(chapterBlocks, chapter.sentences, lastMajorTitle, chapterIndex + 1);
                if (isLikelySectionTitle(chapter.title) && chapter.title.find(" / ") == std::string::npos) {
                    lastMajorTitle = chapter.title;
                }

                outBook.chapters.push_back(std::move(chapter));
                ++chapterIndex;
            }
            continue;
        }

        Chapter chapter;
        chapter.id = idref;
        splitter_.splitBlocks(blocks, chapterIndex, chapter.sentences);
        if (shouldSkipChapter(blocks, chapter.sentences)) {
            continue;
        }

        chapter.title = chooseChapterTitle(blocks, chapter.sentences, lastMajorTitle, chapterIndex + 1);
        if (isLikelySectionTitle(chapter.title) && chapter.title.find(" / ") == std::string::npos) {
            lastMajorTitle = chapter.title;
        }

        outBook.chapters.push_back(std::move(chapter));
        ++chapterIndex;
    }

    return !outBook.chapters.empty();
}

bool EpubCompiler::compileChapterList(const std::string& epubPath, BookScript& outBook) const {
    if (!readMetadata(epubPath, outBook)) {
        return false;
    }

    EpubArchive archive;
    archive.open(epubPath);

    std::string containerXml;
    archive.readTextFile("META-INF/container.xml", containerXml);
    const std::string opfPath = parseRootFilePath(containerXml);
    if (opfPath.empty()) {
        return false;
    }

    std::string opfXml;
    archive.readTextFile(opfPath, opfXml);

    const std::regex manifestPattern("<item\\b[^>]*>", std::regex::icase);
    const std::regex spinePattern(
        "<itemref[^>]*idref\\s*=\\s*\"([^\"]+)\"",
        std::regex::icase);

    std::vector<std::tuple<std::string, std::string, std::string>> manifest;
    for (auto it = std::sregex_iterator(opfXml.begin(), opfXml.end(), manifestPattern);
         it != std::sregex_iterator();
         ++it) {
        const std::string tag = (*it)[0].str();
        const std::string id = parseTagAttribute(tag, "id");
        const std::string href = parseTagAttribute(tag, "href");
        const std::string mediaType = parseTagAttribute(tag, "media-type");
        if (!id.empty() && !href.empty() && !mediaType.empty()) {
            manifest.emplace_back(id, href, mediaType);
        }
    }

    std::vector<std::string> spineIds;
    for (auto it = std::sregex_iterator(opfXml.begin(), opfXml.end(), spinePattern);
         it != std::sregex_iterator();
         ++it) {
        spineIds.push_back((*it)[1].str());
    }

    std::string ncxPath;
    std::string spineTag;
    {
        const std::regex spineTagPattern(R"(<spine\b[^>]*>)", std::regex::icase);
        std::smatch spineMatch;
        if (std::regex_search(opfXml, spineMatch, spineTagPattern)) {
            spineTag = spineMatch[0].str();
        }
    }
    const std::string tocId = parseTagAttribute(spineTag, "toc");
    if (!tocId.empty()) {
        auto tocMatch = std::find_if(manifest.begin(), manifest.end(), [&](const auto& item) {
            return std::get<0>(item) == tocId;
        });
        if (tocMatch != manifest.end()) {
            ncxPath = joinOpfPath(opfPath, std::get<1>(*tocMatch));
        }
    }

    if (!ncxPath.empty()) {
        std::string ncxXml;
        if (archive.readTextFile(ncxPath, ncxXml)) {
            const std::vector<NavPoint> navPoints = parseNavPoints(ncxXml, opfPath);
            for (const NavPoint& navPoint : navPoints) {
                if (!isUsefulNavTitle(navPoint.title)) {
                    continue;
                }

                Chapter chapter;
                chapter.id = navPoint.srcPath + (navPoint.anchor.empty() ? "" : "#" + navPoint.anchor);
                chapter.title = normalizeText(navPoint.title);
                if (!chapter.title.empty()) {
                    outBook.chapters.push_back(std::move(chapter));
                }
            }
            if (!outBook.chapters.empty()) {
                return true;
            }
        }
    }

    std::uint32_t chapterNumber = 1;
    for (const std::string& idref : spineIds) {
        auto match = std::find_if(manifest.begin(), manifest.end(), [&](const auto& item) {
            return std::get<0>(item) == idref;
        });

        if (match == manifest.end()) {
            continue;
        }

        const std::string& href = std::get<1>(*match);
        const std::string& mediaType = std::get<2>(*match);
        if (mediaType != "application/xhtml+xml" && mediaType != "text/html") {
            continue;
        }

        std::string html;
        if (!archive.readTextFile(joinOpfPath(opfPath, href), html)) {
            continue;
        }

        std::vector<TextBlock> blocks;
        if (!extractor_.extract(html, blocks)) {
            continue;
        }

        std::string title;
        for (const TextBlock& block : blocks) {
            title = normalizeText(block.text);
            if (!title.empty()) {
                break;
            }
        }

        if (title.empty()) {
            title = "Chapter " + std::to_string(chapterNumber);
        }

        Chapter chapter;
        chapter.id = idref;
        chapter.title = std::move(title);
        outBook.chapters.push_back(std::move(chapter));
        ++chapterNumber;
    }

    return !outBook.chapters.empty();
}

std::string EpubCompiler::parseRootFilePath(const std::string& containerXml) {
    const std::regex rootFilePattern("full-path\\s*=\\s*\"([^\"]+)\"", std::regex::icase);
    std::smatch match;
    if (std::regex_search(containerXml, match, rootFilePattern)) {
        return match[1].str();
    }
    return {};
}

std::string EpubCompiler::parseBetween(const std::string& text, const std::string& start, const std::string& end) {
    const std::size_t startPos = text.find(start);
    if (startPos == std::string::npos) {
        return {};
    }

    const std::size_t contentPos = text.find('>', startPos);
    if (contentPos == std::string::npos) {
        return {};
    }

    const std::size_t endPos = text.find(end, contentPos + 1);
    if (endPos == std::string::npos || endPos <= contentPos + 1) {
        return {};
    }

    return text.substr(contentPos + 1, endPos - contentPos - 1);
}

std::string EpubCompiler::parseTagAttribute(const std::string& tag, const std::string& attribute) {
    const std::regex pattern(attribute + "\\s*=\\s*\"([^\"]+)\"", std::regex::icase);
    std::smatch match;
    if (std::regex_search(tag, match, pattern)) {
        return match[1].str();
    }
    return {};
}

std::string EpubCompiler::joinOpfPath(const std::string& opfPath, const std::string& href) {
    const auto base = std::filesystem::path(opfPath).parent_path();
    return (base / href).generic_string();
}

std::vector<EpubCompiler::NavPoint> EpubCompiler::parseNavPoints(const std::string& ncxXml, const std::string& opfPath) {
    std::vector<NavPoint> navPoints;
    const std::regex navPointPattern(
        R"NCX(<navPoint\b[\s\S]*?<navLabel>\s*<text>([\s\S]*?)</text>\s*</navLabel>[\s\S]*?<content\b[^>]*src\s*=\s*"([^"]+)")NCX",
        std::regex::icase);

    for (auto it = std::sregex_iterator(ncxXml.begin(), ncxXml.end(), navPointPattern);
         it != std::sregex_iterator();
         ++it) {
        const std::string rawTitle = (*it)[1].str();
        const std::string rawSrc = (*it)[2].str();
        const auto [href, anchor] = splitHrefAnchor(rawSrc);
        if (href.empty()) {
            continue;
        }

        NavPoint navPoint;
        navPoint.title = normalizeText(rawTitle);
        navPoint.srcPath = joinOpfPath(opfPath, href);
        navPoint.anchor = anchor;
        navPoints.push_back(std::move(navPoint));
    }

    return navPoints;
}

std::pair<std::string, std::string> EpubCompiler::splitHrefAnchor(const std::string& href) {
    const std::size_t hashPos = href.find('#');
    if (hashPos == std::string::npos) {
        return {href, {}};
    }
    return {href.substr(0, hashPos), href.substr(hashPos + 1)};
}

std::string EpubCompiler::extractAnchoredSectionHtml(
    const std::string& html,
    const std::string& anchor,
    const std::string& nextAnchor) {
    if (anchor.empty()) {
        return {};
    }

    const std::string doubleQuoted = "id=\"" + anchor + "\"";
    const std::string singleQuoted = "id='" + anchor + "'";
    std::size_t startPos = html.find(doubleQuoted);
    if (startPos == std::string::npos) {
        startPos = html.find(singleQuoted);
    }
    if (startPos == std::string::npos) {
        return {};
    }
    const std::size_t tagStart = html.rfind('<', startPos);
    if (tagStart != std::string::npos) {
        startPos = tagStart;
    }

    std::size_t endPos = html.size();
    if (!nextAnchor.empty()) {
        const std::string nextDoubleQuoted = "id=\"" + nextAnchor + "\"";
        const std::string nextSingleQuoted = "id='" + nextAnchor + "'";
        std::size_t nextPos = html.find(nextDoubleQuoted, startPos + 1);
        if (nextPos == std::string::npos) {
            nextPos = html.find(nextSingleQuoted, startPos + 1);
        }
        if (nextPos != std::string::npos && nextPos > startPos) {
            const std::size_t nextTagStart = html.rfind('<', nextPos);
            endPos = nextTagStart != std::string::npos && nextTagStart > startPos ? nextTagStart : nextPos;
        }
    }

    return html.substr(startPos, endPos - startPos);
}
