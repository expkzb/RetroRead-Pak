#include "ui/LoadingScene.h"

#include <memory>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>

#include "app/Application.h"
#include "core/ProgressStore.h"
#include "epub/EpubCompiler.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "txt/TxtCompiler.h"
#include "ui/BookListScene.h"
#include "ui/ChapterScene.h"
#include "ui/ReaderScene.h"
#include "ui/ThemePalette.h"

namespace {
bool hasExtension(const std::string& path, const char* extension) {
    const std::filesystem::path fsPath(path);
    std::string ext = fsPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext == extension;
}

bool loadOrCompileBook(Application& app, const std::string& path, BookScript& outBook) {
    if (app.bookCache().load(app.fileSystem(), path, outBook)) {
        return true;
    }

    if (hasExtension(path, ".txt")) {
        TxtCompiler compiler;
        if (!compiler.compile(path, outBook)) {
            return false;
        }
    } else {
        EpubCompiler compiler;
        if (!compiler.compile(path, outBook)) {
            return false;
        }
    }

    app.bookCache().save(app.fileSystem(), outBook);
    return true;
}

bool loadChapterList(Application& app, const std::string& path, BookScript& outBook) {
    if (app.bookCache().load(app.fileSystem(), path, outBook)) {
        return true;
    }

    if (hasExtension(path, ".txt")) {
        TxtCompiler compiler;
        return compiler.compile(path, outBook);
    }

    EpubCompiler compiler;
    if (compiler.compileChapterList(path, outBook)) {
        return true;
    }

    return loadOrCompileBook(app, path, outBook);
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

LoadingScene::LoadingScene(
    Application& app,
    std::string bookPath,
    std::string bookTitle,
    LoadingTarget target,
    int requestedChapterIndex)
    : AppScene(app),
      bookPath_(std::move(bookPath)),
      bookTitle_(std::move(bookTitle)),
      target_(target),
      requestedChapterIndex_(requestedChapterIndex) {
}

void LoadingScene::onEnter() {
    started_ = false;
    failed_ = false;
    errorMessage_.clear();
}

void LoadingScene::update(float dt) {
    (void)dt;

    if (failed_) {
        Input& input = app_.input();
        if (input.wasPressed(Action::Back) || input.wasPressed(Action::OpenMenu) || input.wasPressed(Action::Confirm)) {
            app_.sceneManager().replace(std::make_unique<BookListScene>(app_));
        }
        return;
    }

    if (started_) {
        return;
    }

    started_ = true;
    finishLoading();
}

void LoadingScene::finishLoading() {
    BookScript book;
    const bool loaded = target_ == LoadingTarget::Chapters ? loadChapterList(app_, bookPath_, book)
                                                           : loadOrCompileBook(app_, bookPath_, book);
    if (!loaded || book.chapters.empty()) {
        failed_ = true;
        errorMessage_ = "Unable to open this book.";
        return;
    }

    if (target_ == LoadingTarget::Chapters) {
        app_.sceneManager().replace(std::make_unique<ChapterScene>(app_, std::move(book)));
        return;
    }

    ReadingProgress progress{book.bookId, 0, 0, false, app_.settings().textSpeed};
    if (!app_.progressStore().get(book.bookId, progress)) {
        progress.bookId = book.bookId;
        progress.chapterIndex = 0;
        progress.sentenceIndex = 0;
    }
    if (requestedChapterIndex_ >= 0) {
        progress.chapterIndex = static_cast<std::uint32_t>(
            std::min<int>(requestedChapterIndex_, static_cast<int>(book.chapters.size()) - 1));
        progress.sentenceIndex = 0;
    }
    progress.autoPlay = false;
    progress.textSpeed = app_.settings().textSpeed;
    progress.lastOpenedAt = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    app_.progressStore().put(progress);
    app_.progressStore().save(app_.fileSystem());

    app_.sceneManager().replace(std::make_unique<ReaderScene>(app_, std::move(book), progress));
}

void LoadingScene::render(Renderer& renderer) {
    const int screenWidth = renderer.screenWidth();
    const int centerX = screenWidth / 2;
    const ThemePalette palette = themePalette(app_.settings().themePreset);

    renderer.clear(palette.screenBackground);

    renderer.drawText(
        failed_ ? "Open Failed" : "Preparing Book",
        Rect{40, uiSpacing(90, 160), screenWidth - 80, uiSpacing(42, 70)},
        palette.headerText,
        uiFont(28, 52),
        TextAlign::Center,
        app_.settings().fontPreset);

    renderer.drawText(
        bookTitle_,
        Rect{60, uiSpacing(150, 250), screenWidth - 120, uiSpacing(30, 48)},
        palette.primaryText,
        uiFont(18, 32),
        TextAlign::Center,
        app_.settings().fontPreset);

    if (failed_) {
        renderer.drawText(
            errorMessage_,
            Rect{60, uiSpacing(220, 340), screenWidth - 120, uiSpacing(28, 44)},
            palette.accentText,
            uiFont(18, 32),
            TextAlign::Center,
            app_.settings().fontPreset);
        renderer.drawText(
            "A or Menu: back",
            Rect{60, uiSpacing(270, 410), screenWidth - 120, uiSpacing(24, 36)},
            palette.secondaryText,
            uiFont(16, 28),
            TextAlign::Center,
            app_.settings().fontPreset);
        return;
    }

    renderer.fillRect(Rect{centerX - uiSpacing(110, 180), uiSpacing(230, 350), uiSpacing(220, 360), uiSpacing(14, 20)},
                      palette.dialogueInnerBorder);
    renderer.fillRect(Rect{centerX - uiSpacing(110, 180), uiSpacing(230, 350), uiSpacing(150, 240), uiSpacing(14, 20)},
                      palette.dialogueBorder);

    renderer.drawText(
        target_ == LoadingTarget::Reader ? "Building cache and loading your last position..."
                                         : "Building cache and opening the chapter list...",
        Rect{60, uiSpacing(270, 400), screenWidth - 120, uiSpacing(28, 44)},
        palette.secondaryText,
        uiFont(17, 28),
        TextAlign::Center,
        app_.settings().fontPreset);
}
