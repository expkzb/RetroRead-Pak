#include "ui/BookListScene.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <memory>

#include "app/Application.h"
#include "core/ProgressStore.h"
#include "epub/EpubCompiler.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "ui/ChapterScene.h"
#include "ui/LoadingScene.h"
#include "ui/ReaderScene.h"

namespace {
int computePercentRead(const BookScript& book, const ReadingProgress& progress) {
    std::size_t totalSentences = 0;
    for (const Chapter& chapter : book.chapters) {
        totalSentences += chapter.sentences.size();
    }

    if (totalSentences == 0 || book.chapters.empty()) {
        return 0;
    }

    if (progress.chapterIndex >= book.chapters.size()) {
        return 100;
    }

    const std::size_t chapterIndex = std::min<std::size_t>(progress.chapterIndex, book.chapters.size() - 1);
    std::size_t consumed = 0;

    for (std::size_t i = 0; i < chapterIndex; ++i) {
        consumed += book.chapters[i].sentences.size();
    }

    const std::size_t chapterSentenceCount = book.chapters[chapterIndex].sentences.size();
    if (chapterSentenceCount > 0) {
        consumed += std::min<std::size_t>(progress.sentenceIndex + 1, chapterSentenceCount);
    }

    return static_cast<int>((consumed * 100) / totalSentences);
}

int uiFont(int normalSize, int handheldSize) {
#ifdef NEXTREADING_TG5040
    (void)normalSize;
    return handheldSize;
#else
    (void)handheldSize;
    return normalSize;
#endif
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
}  // namespace

BookListScene::BookListScene(Application& app) : AppScene(app) {
}

void BookListScene::onEnter() {
    app_.library().scan(app_.fileSystem());
    books_.clear();

    for (const BookListItem& item : app_.library().books()) {
        BookListEntry entry;
        entry.item = item;
        entry.hasCache = app_.bookCache().exists(app_.fileSystem(), item.path);
        entry.hasProgress = app_.progressStore().get(std::filesystem::path(item.path).stem().string(), entry.progress);
        if (entry.hasCache) {
            BookScript cachedBook;
            if (app_.bookCache().load(app_.fileSystem(), item.path, cachedBook)) {
                entry.author = cachedBook.author;
                entry.chapterCount = cachedBook.chapters.size();
                for (const Chapter& chapter : cachedBook.chapters) {
                    entry.totalSentences += chapter.sentences.size();
                }
                if (entry.hasProgress) {
                    entry.percentRead = computePercentRead(cachedBook, entry.progress);
                }
            } else {
                entry.hasCache = false;
            }
        }

        if (entry.author.empty()) {
            EpubCompiler compiler;
            BookScript metadata;
            if (compiler.readMetadata(item.path, metadata)) {
                entry.author = metadata.author;
            }
        }

        books_.push_back(std::move(entry));
    }

    std::stable_sort(books_.begin(), books_.end(), [](const BookListEntry& lhs, const BookListEntry& rhs) {
        if (lhs.progress.lastOpenedAt != rhs.progress.lastOpenedAt) {
            return lhs.progress.lastOpenedAt > rhs.progress.lastOpenedAt;
        }
        if (lhs.hasProgress != rhs.hasProgress) {
            return lhs.hasProgress > rhs.hasProgress;
        }
        return lhs.item.title < rhs.item.title;
    });

    if (selectedIndex_ >= static_cast<int>(books_.size())) {
        selectedIndex_ = books_.empty() ? 0 : static_cast<int>(books_.size()) - 1;
    }
    clampScroll();
}

void BookListScene::update(float dt) {
    (void)dt;
    Input& input = app_.input();

    if (input.wasPressed(Action::Down) && selectedIndex_ + 1 < static_cast<int>(books_.size())) {
        ++selectedIndex_;
        clampScroll();
    }

    if (input.wasPressed(Action::Up) && selectedIndex_ > 0) {
        --selectedIndex_;
        clampScroll();
    }

    if (input.wasPressed(Action::Back) || input.wasPressed(Action::OpenMenu)) {
        app_.requestQuit();
        return;
    }

    if (input.wasPressed(Action::Confirm)) {
        if (books_.empty()) {
#ifdef NEXTREADING_TG5040
            return;
#else
            app_.sceneManager().replace(std::make_unique<ReaderScene>(
                app_,
                makeDebugBook(),
                ReadingProgress{"debug-book", 0, 0, false, app_.settings().textSpeed}));
            return;
#endif
        }

        app_.sceneManager().replace(std::make_unique<LoadingScene>(
            app_,
            books_[selectedIndex_].item.path,
            books_[selectedIndex_].item.title,
            LoadingTarget::Reader));
        return;
    }

    if (input.wasPressed(Action::Secondary)) {
        if (books_.empty()) {
#ifdef NEXTREADING_TG5040
            return;
#else
            app_.sceneManager().replace(std::make_unique<ChapterScene>(app_, makeDebugBook()));
            return;
#endif
        }

        app_.sceneManager().replace(std::make_unique<LoadingScene>(
            app_,
            books_[selectedIndex_].item.path,
            books_[selectedIndex_].item.title,
            LoadingTarget::Chapters));
    }
}

void BookListScene::render(Renderer& renderer) {
    const int screenWidth = renderer.screenWidth();
    const int leftMargin = std::max(24, screenWidth / 24);
    const int headerWidth = screenWidth - leftMargin * 2;
    const int listWidth = screenWidth - leftMargin * 2 - 12;

    renderer.clear(Color{10, 14, 20, 255});
    renderer.drawText(
        "RetroRead",
        Rect{leftMargin, 24, headerWidth, uiSpacing(40, 84)},
        Color{255, 233, 188, 255},
        uiFont(28, 56),
        TextAlign::Left,
        app_.settings().fontPreset);

    renderer.drawText(
        "A: continue  Y: chapters  Menu: quit",
        Rect{leftMargin, 96, headerWidth, 40},
        Color{174, 194, 214, 255},
        uiFont(16, 32),
        TextAlign::Left,
        app_.settings().fontPreset);

    if (books_.empty()) {
        renderer.drawText(
            "Books folder is empty.",
            Rect{leftMargin, 190, headerWidth, uiSpacing(28, 44)},
            Color{240, 243, 248, 255},
            uiFont(22, 44),
            TextAlign::Left,
            app_.settings().fontPreset);
        renderer.drawText(
            "Put .epub or .txt files into ./Books",
            Rect{leftMargin, 190 + uiSpacing(30, 52), headerWidth, uiSpacing(28, 44)},
            Color{240, 243, 248, 255},
            uiFont(22, 44),
            TextAlign::Left,
            app_.settings().fontPreset);
        return;
    }

    int y = uiSpacing(150, 200);
    const int startIndex = std::max(0, scrollOffset_);
    const int endIndex = std::min(static_cast<int>(books_.size()), startIndex + visibleEntryCount());
    for (int index = startIndex; index < endIndex; ++index) {
        const bool selected = index == selectedIndex_;
        renderer.drawText(
            books_[index].item.title,
            Rect{leftMargin + 24, y, listWidth, uiSpacing(28, 56)},
            selected ? Color{255, 233, 188, 255} : Color{220, 226, 235, 255},
            uiFont(20, 40),
            TextAlign::Left,
            app_.settings().fontPreset);

        renderer.drawText(
            buildListStatus(books_[index]),
            Rect{leftMargin + 44, y + uiSpacing(24, 46), listWidth - 20, uiSpacing(20, 36)},
            selected ? Color{227, 211, 172, 255} : Color{150, 170, 190, 255},
            uiFont(14, 28),
            TextAlign::Left,
            app_.settings().fontPreset);
        y += uiSpacing(48, 88);
    }

    if (app_.settings().performanceMode == PerformanceMode::Hud) {
        renderer.drawText(
            app_.performanceHudText(),
            Rect{renderer.screenWidth() - 360, 18, 320, 24},
            Color{174, 194, 214, 220},
            uiFont(14, 18),
            TextAlign::Right,
            app_.settings().fontPreset);
    }
}

BookScript BookListScene::makeDebugBook() const {
    BookScript book;
    book.bookId = "debug-book";
    book.title = "Debug Book";
    book.author = "RetroRead";

    Chapter chapter;
    chapter.id = "chapter-1";
    chapter.title = "Chapter 1: Rain";
    chapter.sentences.push_back(Sentence{"Chapter 1: Rain", 0, 0, 0, true});
    chapter.sentences.push_back(Sentence{"The room is quiet.", 0, 1, 1, false});
    chapter.sentences.push_back(Sentence{"A distant storm rolls across the town.", 0, 1, 2, false});
    chapter.sentences.push_back(Sentence{"She whispers, \"You finally made it.\"", 0, 2, 3, false});
    book.chapters.push_back(chapter);

    return book;
}

std::string BookListScene::buildListStatus(const BookListEntry& entry) const {
    if (!entry.hasProgress) {
        return "0%";
    }
    if (entry.totalSentences == 0) {
        return "...";
    }
    return std::to_string(entry.percentRead) + "%";
}

int BookListScene::visibleEntryCount() const {
#ifdef NEXTREADING_TG5040
    return 6;
#else
    return 10;
#endif
}

void BookListScene::clampScroll() {
    const int maxOffset = std::max(0, static_cast<int>(books_.size()) - visibleEntryCount());
    if (selectedIndex_ < scrollOffset_) {
        scrollOffset_ = selectedIndex_;
    }
    if (selectedIndex_ >= scrollOffset_ + visibleEntryCount()) {
        scrollOffset_ = selectedIndex_ - visibleEntryCount() + 1;
    }
    scrollOffset_ = std::max(0, std::min(scrollOffset_, maxOffset));
}
