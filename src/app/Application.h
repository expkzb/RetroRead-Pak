#pragma once

#include <memory>

#include "audio/TextBlipPlayer.h"
#include "core/BookCache.h"
#include "core/BookLibrary.h"
#include "core/ProgressStore.h"
#include "core/SettingsStore.h"
#include "core/BookTypes.h"
#include "platform/Clock.h"
#include "platform/FileSystem.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "ui/SceneManager.h"

class Application {
public:
    Application(
        std::unique_ptr<Renderer> renderer,
        std::unique_ptr<Input> input,
        std::unique_ptr<FileSystem> fileSystem,
        std::unique_ptr<Clock> clock);

    bool initialize();
    void run();
    void shutdown();

    bool isRunning() const;
    void requestQuit();

    Renderer& renderer();
    Input& input();
    FileSystem& fileSystem();
    Clock& clock();
    SceneManager& sceneManager();
    BookLibrary& library();
    BookCache& bookCache();
    ProgressStore& progressStore();
    SettingsStore& settingsStore();
    TextBlipPlayer& textBlipPlayer();

    const ReaderSettings& settings() const;
    ReaderSettings& settings();

private:
    bool running_ = false;
    ReaderSettings settings_;
    BookLibrary library_;
    BookCache bookCache_;
    ProgressStore progressStore_;
    SettingsStore settingsStore_;
    TextBlipPlayer textBlipPlayer_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Input> input_;
    std::unique_ptr<FileSystem> fileSystem_;
    std::unique_ptr<Clock> clock_;
    SceneManager sceneManager_;
};
