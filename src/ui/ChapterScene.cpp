#include "ui/ChapterScene.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>

#include "app/Application.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "ui/BookListScene.h"
#include "ui/LoadingScene.h"
#include "ui/ReaderScene.h"
#include "ui/ThemePalette.h"

namespace {
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
}

ChapterScene::ChapterScene(Application& app, BookScript book)
    : AppScene(app), book_(std::move(book)) {
}

void ChapterScene::onEnter() {
    hasSavedProgress_ = app_.progressStore().get(book_.bookId, savedProgress_);
    if (hasSavedProgress_ && savedProgress_.chapterIndex < book_.chapters.size()) {
        selectedIndex_ = static_cast<int>(savedProgress_.chapterIndex);
    }

    if (selectedIndex_ >= static_cast<int>(book_.chapters.size())) {
        selectedIndex_ = book_.chapters.empty() ? 0 : static_cast<int>(book_.chapters.size()) - 1;
    }
}

void ChapterScene::update(float dt) {
    (void)dt;
    Input& input = app_.input();

    if (input.wasPressed(Action::Down) && selectedIndex_ + 1 < static_cast<int>(book_.chapters.size())) {
        ++selectedIndex_;
    }

    if (input.wasPressed(Action::Up) && selectedIndex_ > 0) {
        --selectedIndex_;
    }

    if (input.wasPressed(Action::Back) || input.wasPressed(Action::OpenMenu)) {
        app_.sceneManager().replace(std::make_unique<BookListScene>(app_));
        return;
    }

    if (input.wasPressed(Action::Confirm) && !book_.chapters.empty()) {
        if (book_.chapters[selectedIndex_].sentences.empty() && !book_.sourcePath.empty()) {
            app_.sceneManager().replace(std::make_unique<LoadingScene>(
                app_,
                book_.sourcePath,
                book_.title,
                LoadingTarget::Reader,
                selectedIndex_));
            return;
        }

        ReadingProgress progress{
            book_.bookId,
            static_cast<std::uint32_t>(selectedIndex_),
            0,
            false,
            app_.settings().textSpeed};

        if (hasSavedProgress_) {
            progress.autoPlay = savedProgress_.autoPlay;
            progress.textSpeed = savedProgress_.textSpeed;
            if (savedProgress_.chapterIndex == static_cast<std::uint32_t>(selectedIndex_)) {
                progress.sentenceIndex = savedProgress_.sentenceIndex;
            }
        }
        progress.lastOpenedAt = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
        app_.progressStore().put(progress);
        app_.progressStore().save(app_.fileSystem());

        app_.sceneManager().replace(std::make_unique<ReaderScene>(
            app_,
            book_,
            progress));
    }
}

void ChapterScene::render(Renderer& renderer) {
    const ThemePalette palette = themePalette(app_.settings().themePreset);

    renderer.clear(palette.screenBackground);
    renderer.drawText(
        book_.title,
        Rect{48, 40, renderer.screenWidth() - 96, uiSpacing(36, 60)},
        palette.headerText,
        uiFont(26, 48),
        TextAlign::Left,
        app_.settings().fontPreset);

    renderer.drawText(
        "A: open  Menu: back",
        Rect{48, 100, renderer.screenWidth() - 96, uiSpacing(24, 40)},
        palette.secondaryText,
        uiFont(16, 30),
        TextAlign::Left,
        app_.settings().fontPreset);

    if (book_.chapters.empty()) {
        renderer.drawText(
            "No readable chapters found.",
            Rect{48, 180, renderer.screenWidth() - 96, uiSpacing(28, 52)},
            palette.primaryText,
            uiFont(22, 40),
            TextAlign::Left,
            app_.settings().fontPreset);
        return;
    }

    const int maxVisible = uiSpacing(16, 9);
    const int startIndex = std::max(0, selectedIndex_ - uiSpacing(7, 4));
    const int endIndex = std::min(static_cast<int>(book_.chapters.size()), startIndex + maxVisible);

    int y = uiSpacing(132, 180);
    for (int i = startIndex; i < endIndex; ++i) {
        const bool selected = i == selectedIndex_;
        const Color textColor = selected ? palette.selectionText : palette.primaryText;
        const Color metaColor = selected ? palette.selectionSubtext : palette.secondaryText;
        if (selected) {
            const Rect rowRect{52, y - uiSpacing(4, 6), renderer.screenWidth() - 104, uiSpacing(50, 80)};
            renderer.fillRect(rowRect, palette.selectionFill);
            renderer.drawRect(rowRect, palette.selectionOutline);
        }

        renderer.drawText(
            std::to_string(i + 1) + ". " + book_.chapters[i].title,
            Rect{64, y + uiSpacing(2, 4), renderer.screenWidth() - 128, uiSpacing(26, 48)},
            textColor,
            uiFont(20, 38),
            TextAlign::Left,
            app_.settings().fontPreset);

        if (!book_.chapters[i].sentences.empty()) {
            renderer.drawText(
                std::to_string(book_.chapters[i].sentences.size()) + " sentences",
                Rect{90, y + uiSpacing(26, 46), renderer.screenWidth() - 180, uiSpacing(20, 32)},
                metaColor,
                uiFont(14, 24),
                TextAlign::Left,
                app_.settings().fontPreset);
        }

        if (hasSavedProgress_ && savedProgress_.chapterIndex == static_cast<std::uint32_t>(i)) {
            renderer.drawText(
                "resume",
                Rect{renderer.screenWidth() - uiSpacing(160, 220), y, uiSpacing(100, 160), uiSpacing(20, 32)},
                selected ? palette.selectionText : palette.secondaryText,
                uiFont(14, 24),
                TextAlign::Right,
                app_.settings().fontPreset);
        }

        y += uiSpacing(46, 82);
    }

    if (app_.settings().performanceMode == PerformanceMode::Hud) {
        renderer.drawText(
            app_.performanceHudText(),
            Rect{renderer.screenWidth() - 360, 18, 320, 24},
            palette.secondaryText,
            uiFont(14, 18),
            TextAlign::Right,
            app_.settings().fontPreset);
    }
}
