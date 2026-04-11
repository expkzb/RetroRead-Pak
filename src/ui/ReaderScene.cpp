#include "ui/ReaderScene.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

#include "app/Application.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "text/Utf8.h"
#include "ui/BookListScene.h"
#include "ui/SettingsScene.h"

namespace {
bool isLineStartForbiddenPunctuation(const std::string& cp) {
    return cp == "," || cp == "." || cp == "!" || cp == "?" || cp == ":" || cp == ";" || cp == ")" ||
           cp == "]" || cp == "}" || cp == u8"\uff0c" || cp == u8"\u3002" || cp == u8"\u3001" ||
           cp == u8"\uff01" || cp == u8"\uff1f" || cp == u8"\uff1a" || cp == u8"\uff1b" ||
           cp == u8"\u201d" || cp == u8"\u2019" || cp == u8"\u300d" || cp == u8"\u300f" ||
           cp == u8"\u300b" || cp == u8"\uff09" || cp == u8"\u3011" || cp == u8"\u2026";
}

void pullLeadingPunctuationBackward(std::vector<std::string>& lines) {
    for (std::size_t i = 1; i < lines.size(); ++i) {
        auto current = utf8::splitCodepoints(lines[i]);
        if (current.empty()) {
            continue;
        }

        std::size_t moveCount = 0;
        while (moveCount < current.size() && isLineStartForbiddenPunctuation(current[moveCount])) {
            ++moveCount;
        }

        if (moveCount == 0) {
            continue;
        }

        lines[i - 1] += utf8::join(current, 0, moveCount);
        lines[i] = utf8::join(current, moveCount, current.size());
    }

    lines.erase(std::remove_if(lines.begin(), lines.end(), [](const std::string& line) { return line.empty(); }), lines.end());
}

[[maybe_unused]] std::vector<std::string> wrapTextSimple(
    const std::string& text,
    int maxWidth,
    int fontSize,
    FontPreset fontPreset,
    const Renderer& renderer) {
    std::vector<std::string> lines;
    const auto codepoints = utf8::splitCodepoints(text);
    std::string currentLine;
    std::size_t lastBreakIndex = std::string::npos;
    std::size_t lineStart = 0;

    auto flushLine = [&](std::size_t begin, std::size_t end) {
        std::string line = utf8::join(codepoints, begin, end);
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
            line.erase(line.begin());
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    };

    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        currentLine += codepoints[i];
        if (utf8::isWhitespace(codepoints[i]) || codepoints[i] == "，" || codepoints[i] == "。" ||
            codepoints[i] == "、" || codepoints[i] == "," || codepoints[i] == ".") {
            lastBreakIndex = i;
        }

        if (renderer.measureTextWidth(currentLine, fontSize, fontPreset) <= maxWidth) {
            continue;
        }

        if (lastBreakIndex != std::string::npos && lastBreakIndex >= lineStart) {
            flushLine(lineStart, lastBreakIndex + 1);
            lineStart = lastBreakIndex + 1;
            while (lineStart < codepoints.size() && utf8::isWhitespace(codepoints[lineStart])) {
                ++lineStart;
            }
            i = lineStart > 0 ? lineStart - 1 : 0;
        } else {
            flushLine(lineStart, i);
            lineStart = i;
            i = lineStart > 0 ? lineStart - 1 : 0;
        }

        currentLine.clear();
        lastBreakIndex = std::string::npos;
    }

    if (lineStart < codepoints.size()) {
        flushLine(lineStart, codepoints.size());
    }

    if (lines.empty() && !text.empty()) {
        lines.push_back(text);
    }

    pullLeadingPunctuationBackward(lines);
    return lines;
}

std::vector<std::string> wrapTextSmart(
    const std::string& text,
    int maxWidth,
    int fontSize,
    FontPreset fontPreset,
    const Renderer& renderer) {
    std::vector<std::string> lines;
    const auto codepoints = utf8::splitCodepoints(text);
    std::size_t lineStart = 0;
    std::size_t lastBreakIndex = std::string::npos;
    std::string currentLine;

    auto isAsciiWordChar = [](const std::string& cp) {
        if (cp.size() != 1) {
            return false;
        }
        const unsigned char ch = static_cast<unsigned char>(cp[0]);
        return std::isalnum(ch) != 0 || ch == '\'' || ch == '_';
    };

    auto isBreakChar = [&](const std::string& cp) {
        return utf8::isWhitespace(cp) || cp == "," || cp == "." || cp == "!" || cp == "?" || cp == ":" ||
               cp == ";" || cp == ")" || cp == "]" || cp == "}" || cp == "-" || cp == "/" || cp == u8"，" ||
               cp == u8"。" || cp == u8"！" || cp == u8"？" || cp == u8"：" || cp == u8"；" || cp == u8"、";
    };

    auto flushLine = [&](std::size_t begin, std::size_t end) {
        const auto lineCodepoints = utf8::splitCodepoints(utf8::join(codepoints, begin, end));
        std::size_t trimBegin = 0;
        std::size_t trimEnd = lineCodepoints.size();
        while (trimBegin < trimEnd && utf8::isWhitespace(lineCodepoints[trimBegin])) {
            ++trimBegin;
        }
        while (trimEnd > trimBegin && utf8::isWhitespace(lineCodepoints[trimEnd - 1])) {
            --trimEnd;
        }
        const std::string line = utf8::join(lineCodepoints, trimBegin, trimEnd);
        if (!line.empty()) {
            lines.push_back(line);
        }
    };

    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        currentLine += codepoints[i];
        if (isBreakChar(codepoints[i]) || !isAsciiWordChar(codepoints[i])) {
            lastBreakIndex = i;
        }

        if (renderer.measureTextWidth(currentLine, fontSize, fontPreset) <= maxWidth) {
            continue;
        }

        std::size_t breakIndex = lastBreakIndex;
        if (breakIndex == std::string::npos || breakIndex < lineStart) {
            std::size_t wordStart = i;
            while (wordStart > lineStart && isAsciiWordChar(codepoints[wordStart - 1])) {
                --wordStart;
            }
            if (wordStart > lineStart) {
                breakIndex = wordStart - 1;
            }
        }

        if (breakIndex != std::string::npos && breakIndex >= lineStart) {
            flushLine(lineStart, breakIndex + 1);
            lineStart = breakIndex + 1;
            while (lineStart < codepoints.size() && utf8::isWhitespace(codepoints[lineStart])) {
                ++lineStart;
            }
            i = lineStart > 0 ? lineStart - 1 : 0;
        } else {
            flushLine(lineStart, i);
            lineStart = i;
            i = lineStart > 0 ? lineStart - 1 : 0;
        }

        currentLine.clear();
        lastBreakIndex = std::string::npos;
    }

    if (lineStart < codepoints.size()) {
        flushLine(lineStart, codepoints.size());
    }

    if (lines.empty() && !text.empty()) {
        lines.push_back(text);
    }

    pullLeadingPunctuationBackward(lines);
    return lines;
}

int uiSpacing(int normalValue, int handheldValue) {
#ifdef NEXTREADING_TG5040
    (void)normalValue;
    return handheldValue;
#else
    (void)handheldValue;
    return normalValue;
#endif
}

int readerBodyFont(const ReaderSettings& settings) {
#ifdef NEXTREADING_TG5040
    return std::max(24, std::min(48, static_cast<int>(settings.fontSize) + 8));
#else
    return std::max(20, std::min(36, static_cast<int>(settings.fontSize)));
#endif
}

int readerHeaderFont() {
#ifdef NEXTREADING_TG5040
    return 38;
#else
    return 26;
#endif
}

int readerStatusFont() {
#ifdef NEXTREADING_TG5040
    return 24;
#else
    return 18;
#endif
}

int readerMetaFont() {
#ifdef NEXTREADING_TG5040
    return 20;
#else
    return 15;
#endif
}

}  // namespace

ReaderScene::ReaderScene(Application& app, BookScript book, ReadingProgress progress)
    : AppScene(app), book_(std::move(book)), progress_(std::move(progress)) {
    if (book_.chapters.empty()) {
        progress_.chapterIndex = 0;
        progress_.sentenceIndex = 0;
        return;
    }

    if (progress_.chapterIndex >= book_.chapters.size()) {
        return;
    }

    const Chapter& chapter = book_.chapters[progress_.chapterIndex];
    if (chapter.sentences.empty()) {
        progress_.sentenceIndex = 0;
    } else if (progress_.sentenceIndex >= chapter.sentences.size()) {
        progress_.sentenceIndex = static_cast<std::uint32_t>(chapter.sentences.size() - 1);
    }
}

void ReaderScene::onEnter() {
    maxVisibleLines_ = 4;
    invalidateLayoutCache();
    renderRequested_ = true;
    revealCurrentSentence();
}

void ReaderScene::onExit() {
    persistProgress();
}

void ReaderScene::update(float dt) {
    app_.textBlipPlayer().update(dt);
    handleInput();

    switch (state_) {
    case ReaderState::Typing:
        updateTyping(dt);
        break;
    case ReaderState::AutoAdvanceDelay:
        autoAdvanceTimer_ -= dt;
        if (autoAdvanceTimer_ <= 0.0f) {
            if (hasMorePagedLines(app_.renderer())) {
                advancePage(app_.renderer());
                autoAdvanceTimer_ = sentencePauseSeconds_;
            } else {
                moveToNextSentence();
            }
        }
        break;
    case ReaderState::WaitingForInput:
    case ReaderState::Paused:
    case ReaderState::Finished:
        break;
    }
}

void ReaderScene::render(Renderer& renderer) {
    const int screenWidth = renderer.screenWidth();
    const int screenHeight = renderer.screenHeight();
    const int horizontalMargin = std::max(18, screenWidth / 24);
    const int headerWidth = std::max(240, screenWidth - horizontalMargin * 2);
    const int dialogueHeight = uiSpacing(std::max(180, std::min(236, screenHeight / 3)),
                                         std::max(300, std::min(420, (screenHeight * 11) / 20)));
    const int dialogueY = screenHeight - dialogueHeight - std::max(18, screenHeight / 28);
    const int dialogueWidth = screenWidth - horizontalMargin * 2;
    dialogueBox_.setBounds(Rect{horizontalMargin, dialogueY, dialogueWidth, dialogueHeight});

    renderer.clear(Color{20, 24, 34, 255});

    const Chapter* chapter = currentChapter();
    const Sentence* sentence = currentSentence();
    const ReaderSettings& settings = app_.settings();
    refreshLayoutCache(renderer);
    maxVisibleLines_ = visibleLineCapacity(renderer);
    const std::string status =
        chapter != nullptr
            ? "Chapter " + std::to_string(progress_.chapterIndex + 1) + "/" + std::to_string(book_.chapters.size()) +
                  "  Sentence " + std::to_string(progress_.sentenceIndex + 1) + "/" +
                  std::to_string(chapter->sentences.size()) + (progress_.autoPlay ? "  AUTO" : "  MANUAL")
            : "Finished";
    const std::string hint =
        progress_.autoPlay ? "AUTO  L1/R1 chapter  Start settings  Menu back"
                           : "A next  X auto  L1/R1 chapter  Start settings  Menu back";

    renderer.drawText(
        book_.title,
        Rect{horizontalMargin, uiSpacing(std::max(20, screenHeight / 22), 24), headerWidth, uiSpacing(40, 58)},
        Color{255, 233, 188, 255},
        readerHeaderFont(),
        TextAlign::Left,
        settings.fontPreset);

    if (!book_.author.empty()) {
        renderer.drawText(
            book_.author,
            Rect{horizontalMargin, uiSpacing(std::max(58, screenHeight / 13), 76), headerWidth, uiSpacing(22, 30)},
            Color{154, 172, 192, 255},
            readerMetaFont(),
            TextAlign::Left,
            settings.fontPreset);
    }

    renderer.drawText(
        status,
        Rect{horizontalMargin, uiSpacing(std::max(92, screenHeight / 8), 124), headerWidth, uiSpacing(24, 32)},
        Color{174, 194, 214, 255},
        readerStatusFont(),
        TextAlign::Left,
        settings.fontPreset);

    if (settings.performanceMode == PerformanceMode::Hud) {
        renderer.drawText(
            app_.performanceHudText(),
            Rect{screenWidth - horizontalMargin - 340, uiSpacing(std::max(24, screenHeight / 22), 28), 320, 24},
            Color{174, 194, 214, 220},
            readerMetaFont(),
            TextAlign::Right,
            settings.fontPreset);
    }

    if (state_ == ReaderState::Finished || sentence == nullptr) {
        dialogueBox_.setTitle("The End");
        dialogueBox_.setBodyLines({"This book has reached the end.", "Press Menu to return."});
        dialogueBox_.setHint("Menu back");
        dialogueBox_.render(renderer, settings);
        return;
    }

    dialogueBox_.setTitle(chapter->title);
    dialogueBox_.setBodyLines(wrapVisibleText(renderer));
    dialogueBox_.setBodyRevealTexts(visibleRevealTextsOnCurrentPage(renderer));
    dialogueBox_.setHint(hint);
    dialogueBox_.render(renderer, settings);
}

bool ReaderScene::shouldRenderContinuously() const {
    return state_ == ReaderState::Typing || state_ == ReaderState::AutoAdvanceDelay ||
           app_.settings().performanceMode == PerformanceMode::Hud;
}

bool ReaderScene::consumeRenderRequest() {
    const bool requested = renderRequested_;
    renderRequested_ = false;
    return requested;
}

void ReaderScene::handleInput() {
    Input& input = app_.input();
    Renderer& renderer = app_.renderer();

    if (input.wasPressed(Action::OpenMenu) || input.wasPressed(Action::Back)) {
        persistProgress();
        app_.sceneManager().replace(std::make_unique<BookListScene>(app_));
        return;
    }

    if (input.wasPressed(Action::Start)) {
        persistProgress();
        app_.sceneManager().replace(std::make_unique<SettingsScene>(app_, book_, progress_));
        return;
    }

    if (input.wasPressed(Action::ToggleAuto)) {
        progress_.autoPlay = !progress_.autoPlay;
        persistProgress();
        if (state_ == ReaderState::WaitingForInput && progress_.autoPlay) {
            state_ = ReaderState::AutoAdvanceDelay;
            autoAdvanceTimer_ = sentencePauseSeconds_;
        }
    }

    if (input.wasPressed(Action::PrevChapter)) {
        moveToPreviousChapter();
        return;
    }

    if (input.wasPressed(Action::NextChapter)) {
        moveToNextChapter();
        return;
    }

    if (input.wasPressed(Action::FastForward)) {
        typer_.revealAll();
        state_ = progress_.autoPlay ? ReaderState::AutoAdvanceDelay : ReaderState::WaitingForInput;
        autoAdvanceTimer_ = sentencePauseSeconds_;
        return;
    }

    if (input.wasPressed(Action::Left) || input.wasPressed(Action::Up)) {
        moveToPreviousSentence();
        return;
    }

    if (input.wasPressed(Action::Confirm)) {
        if (state_ == ReaderState::Typing) {
            if (canAdvanceTypedPage(renderer)) {
                advancePage(renderer);
                return;
            }
            typer_.revealAll();
            state_ = progress_.autoPlay ? ReaderState::AutoAdvanceDelay : ReaderState::WaitingForInput;
            autoAdvanceTimer_ = sentencePauseSeconds_;
            return;
        }

        if (state_ == ReaderState::WaitingForInput) {
            if (hasMorePagedLines(renderer)) {
                advancePage(renderer);
                return;
            }
            moveToNextSentence();
        }
    }
}

void ReaderScene::updateTyping(float dt) {
    const std::size_t previousVisible = typer_.visibleChars();
    const int advanced = typer_.update(dt);
    if (advanced > 0) {
        renderRequested_ = true;
    }
    if (app_.settings().textVoiceMode == TextVoiceMode::Fixed) {
        app_.textBlipPlayer().syncVisibleCodepoints(typer_.codepoints(), typer_.visibleChars());
    } else if (app_.settings().textVoiceMode == TextVoiceMode::FollowText && advanced > 0) {
        const auto& codepoints = typer_.codepoints();
        for (int i = 0; i < advanced; ++i) {
            const std::size_t index = previousVisible + static_cast<std::size_t>(i);
            if (index < codepoints.size()) {
                const std::string previous = index > 0 ? codepoints[index - 1] : std::string{};
                app_.textBlipPlayer().playCodepoint(codepoints[index], previous);
            }
        }
    }
    if (!typer_.isComplete()) {
        return;
    }

    state_ = progress_.autoPlay ? ReaderState::AutoAdvanceDelay : ReaderState::WaitingForInput;
    autoAdvanceTimer_ = sentencePauseSeconds_;
    renderRequested_ = true;
}

void ReaderScene::revealCurrentSentence() {
    const Sentence* sentence = currentSentence();
    if (sentence == nullptr) {
        state_ = ReaderState::Finished;
        return;
    }

    invalidateLayoutCache();
    app_.textBlipPlayer().reset();
    typer_.start(sentence->text, static_cast<int>(progress_.textSpeed));
    pageStartLine_ = 0;
    state_ = ReaderState::Typing;
    renderRequested_ = true;
}

void ReaderScene::moveToNextSentence() {
    const Chapter* chapter = currentChapter();
    if (chapter == nullptr) {
        state_ = ReaderState::Finished;
        return;
    }

    ++progress_.sentenceIndex;
    if (progress_.sentenceIndex >= chapter->sentences.size()) {
        ++progress_.chapterIndex;
        progress_.sentenceIndex = 0;
    }

    if (progress_.chapterIndex >= book_.chapters.size()) {
        persistProgress();
        state_ = ReaderState::Finished;
        return;
    }

    persistProgress();
    revealCurrentSentence();
}

void ReaderScene::moveToPreviousSentence() {
    if (book_.chapters.empty()) {
        return;
    }

    if (progress_.sentenceIndex > 0) {
        --progress_.sentenceIndex;
        pageStartLine_ = 0;
        persistProgress();
        revealCurrentSentence();
        return;
    }

    if (progress_.chapterIndex == 0) {
        return;
    }

    --progress_.chapterIndex;
    const Chapter* chapter = currentChapter();
    if (chapter == nullptr || chapter->sentences.empty()) {
        progress_.sentenceIndex = 0;
    } else {
        progress_.sentenceIndex = static_cast<std::uint32_t>(chapter->sentences.size() - 1);
    }
    pageStartLine_ = 0;
    persistProgress();
    revealCurrentSentence();
}

void ReaderScene::moveToPreviousChapter() {
    if (book_.chapters.empty() || progress_.chapterIndex == 0) {
        return;
    }

    --progress_.chapterIndex;
    progress_.sentenceIndex = 0;
    persistProgress();
    revealCurrentSentence();
}

void ReaderScene::moveToNextChapter() {
    if (book_.chapters.empty() || progress_.chapterIndex + 1 >= book_.chapters.size()) {
        return;
    }

    ++progress_.chapterIndex;
    progress_.sentenceIndex = 0;
    persistProgress();
    revealCurrentSentence();
}

std::vector<std::string> ReaderScene::wrapVisibleText(Renderer& renderer) const {
    const std::vector<std::string>& lines = const_cast<ReaderScene*>(this)->allVisibleLines(renderer);
    if (lines.empty() || pageStartLine_ >= lines.size()) {
        return lines;
    }

    const std::size_t end = std::min(lines.size(), pageStartLine_ + maxVisibleLines_);
    return std::vector<std::string>(lines.begin() + static_cast<std::ptrdiff_t>(pageStartLine_),
                                    lines.begin() + static_cast<std::ptrdiff_t>(end));
}

const Chapter* ReaderScene::currentChapter() const {
    if (progress_.chapterIndex >= book_.chapters.size()) {
        return nullptr;
    }

    return &book_.chapters[progress_.chapterIndex];
}

const Sentence* ReaderScene::currentSentence() const {
    const Chapter* chapter = currentChapter();
    if (chapter == nullptr || progress_.sentenceIndex >= chapter->sentences.size()) {
        return nullptr;
    }

    return &chapter->sentences[progress_.sentenceIndex];
}

bool ReaderScene::hasMorePagedLines(Renderer& renderer) const {
    const std::vector<std::string>& lines = const_cast<ReaderScene*>(this)->allVisibleLines(renderer);
    const std::size_t capacity = visibleLineCapacity(renderer);
    return pageStartLine_ + capacity < lines.size();
}

bool ReaderScene::canAdvanceTypedPage(Renderer& renderer) const {
    if (!hasMorePagedLines(renderer)) {
        return false;
    }
    return typer_.visibleChars() >= visibleCharsOnCurrentPage(renderer) && !typer_.isComplete();
}

void ReaderScene::advancePage(Renderer& renderer) {
    const std::vector<std::string>& lines = allVisibleLines(renderer);
    const std::size_t capacity = visibleLineCapacity(renderer);
    if (pageStartLine_ + capacity < lines.size()) {
        pageStartLine_ += capacity;
        renderRequested_ = true;
    }
}

const std::vector<std::string>& ReaderScene::allVisibleLines(Renderer& renderer) {
    refreshLayoutCache(renderer);
    return cachedWrappedLines_;
}

void ReaderScene::refreshLayoutCache(Renderer& renderer) {
    const Sentence* sentence = currentSentence();
    if (sentence == nullptr) {
        cachedWrappedLines_.clear();
        cachedLineCharCounts_.clear();
        cachedLineCodepoints_.clear();
        cachedSentenceText_.clear();
        layoutCacheValid_ = true;
        return;
    }

    const int bodyFont = readerBodyFont(app_.settings());
    const int dialogueMargin = std::max(18, renderer.screenWidth() / 24);
    const int dialogueWidth = renderer.screenWidth() - dialogueMargin * 2;
    const int safetyPadding = std::max(bodyFont / 2, uiSpacing(18, 30));
    const int textWidth = std::max(220, dialogueWidth - 48 - safetyPadding);
    const FontPreset fontPreset = app_.settings().fontPreset;

    if (layoutCacheValid_ && cachedSentenceText_ == sentence->text && cachedTextWidth_ == textWidth &&
        cachedBodyFont_ == bodyFont && cachedFontPreset_ == fontPreset) {
        return;
    }

    cachedSentenceText_ = sentence->text;
    cachedTextWidth_ = textWidth;
    cachedBodyFont_ = bodyFont;
    cachedFontPreset_ = fontPreset;
    cachedWrappedLines_ = wrapTextSmart(sentence->text, textWidth, bodyFont, fontPreset, renderer);
    cachedLineCharCounts_.clear();
    cachedLineCodepoints_.clear();
    cachedLineCharCounts_.reserve(cachedWrappedLines_.size());
    cachedLineCodepoints_.reserve(cachedWrappedLines_.size());
    for (const std::string& line : cachedWrappedLines_) {
        const auto codepoints = utf8::splitCodepoints(line);
        cachedLineCharCounts_.push_back(codepoints.size());
        cachedLineCodepoints_.push_back(codepoints);
    }
    layoutCacheValid_ = true;
}

void ReaderScene::invalidateLayoutCache() {
    layoutCacheValid_ = false;
}

std::vector<std::string> ReaderScene::visibleRevealTextsOnCurrentPage(Renderer& renderer) const {
    const Sentence* sentence = currentSentence();
    if (sentence == nullptr) {
        return {};
    }

    const_cast<ReaderScene*>(this)->allVisibleLines(renderer);

    std::size_t consumedBeforePage = 0;
    for (std::size_t i = 0; i < std::min(pageStartLine_, cachedLineCharCounts_.size()); ++i) {
        consumedBeforePage += cachedLineCharCounts_[i];
    }

    std::size_t remainingVisible =
        typer_.visibleChars() > consumedBeforePage ? typer_.visibleChars() - consumedBeforePage : 0;
    const std::size_t pageEnd = std::min(pageStartLine_ + maxVisibleLines_, cachedWrappedLines_.size());
    std::vector<std::string> texts;
    texts.reserve(pageEnd > pageStartLine_ ? pageEnd - pageStartLine_ : 0);

    for (std::size_t i = pageStartLine_; i < pageEnd; ++i) {
        const std::size_t visibleCount = std::min<std::size_t>(remainingVisible, cachedLineCharCounts_[i]);
        texts.push_back(utf8::join(cachedLineCodepoints_[i], 0, visibleCount));
        remainingVisible = remainingVisible > cachedLineCharCounts_[i] ? remainingVisible - cachedLineCharCounts_[i] : 0;
    }

    return texts;
}

std::size_t ReaderScene::visibleCharsOnCurrentPage(Renderer& renderer) const {
    const_cast<ReaderScene*>(this)->allVisibleLines(renderer);
    if (cachedLineCharCounts_.empty() || pageStartLine_ >= cachedLineCharCounts_.size()) {
        return 0;
    }

    const std::size_t pageEnd = std::min(pageStartLine_ + visibleLineCapacity(renderer), cachedLineCharCounts_.size());
    std::size_t visibleChars = 0;
    for (std::size_t i = pageStartLine_; i < pageEnd; ++i) {
        visibleChars += cachedLineCharCounts_[i];
    }
    return visibleChars;
}

std::size_t ReaderScene::visibleLineCapacity(Renderer& renderer) const {
    const ReaderSettings& settings = app_.settings();
    const int screenHeight = renderer.screenHeight();
    const int dialogueHeight = uiSpacing(std::max(180, std::min(236, screenHeight / 3)),
                                         std::max(300, std::min(420, (screenHeight * 11) / 20)));
    const int titleHeight = renderer.lineHeight(30, settings.fontPreset);
    const int bodyHeight = renderer.lineHeight(readerBodyFont(settings), settings.fontPreset);
    const int hintHeight = renderer.lineHeight(24, settings.fontPreset);
    const int reserved =
        16 + titleHeight + uiSpacing(14, 22) + hintHeight + uiSpacing(10, 14) + uiSpacing(10, 16);
    const int availableBody = std::max(bodyHeight, dialogueHeight - reserved);
    const int lineStep = bodyHeight + uiSpacing(6, 12);
    return std::max<std::size_t>(1, static_cast<std::size_t>(std::max(1, availableBody / std::max(1, lineStep))));
}

void ReaderScene::persistProgress() {
    app_.progressStore().put(progress_);
    app_.progressStore().save(app_.fileSystem());
}
