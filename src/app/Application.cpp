#include "app/Application.h"

#include <chrono>
#include <memory>
#include <sstream>
#include <utility>

#include "ui/BookListScene.h"

Application::Application(
    std::unique_ptr<Renderer> renderer,
    std::unique_ptr<Input> input,
    std::unique_ptr<FileSystem> fileSystem,
    std::unique_ptr<Clock> clock)
    : renderer_(std::move(renderer)),
      input_(std::move(input)),
      fileSystem_(std::move(fileSystem)),
      clock_(std::move(clock)) {
}

bool Application::initialize() {
    if (!renderer_ || !input_ || !fileSystem_ || !clock_) {
        return false;
    }

    if (!renderer_->initialize() || !input_->initialize() || !fileSystem_->initialize()) {
        return false;
    }
    textBlipPlayer_.initialize();

    settings_ = ReaderSettings{};
    settingsStore_.load(*fileSystem_, settings_);
    library_.scan(*fileSystem_);
    progressStore_.load(*fileSystem_);
    sceneManager_.setRoot(std::make_unique<BookListScene>(*this));
    running_ = true;
    return true;
}

void Application::run() {
    while (running_) {
        const float dt = clock_->tick();

        input_->poll();
        if (input_->quitRequested()) {
            requestQuit();
        }

        sceneManager_.update(dt);

        renderer_->beginFrame();
        sceneManager_.render(*renderer_);
        if (input_->wasPressed(Action::Screenshot)) {
            const std::string screenshotPath = buildScreenshotPath();
            if (!screenshotPath.empty()) {
                renderer_->saveScreenshot(screenshotPath);
            }
        }
        renderer_->endFrame();
    }
}

void Application::shutdown() {
    if (fileSystem_) {
        progressStore_.save(*fileSystem_);
        settingsStore_.save(*fileSystem_, settings_);
    }
    if (input_) {
        input_->shutdown();
    }
    textBlipPlayer_.shutdown();
    if (renderer_) {
        renderer_->shutdown();
    }
    running_ = false;
}

bool Application::isRunning() const {
    return running_;
}

void Application::requestQuit() {
    running_ = false;
}

Renderer& Application::renderer() {
    return *renderer_;
}

Input& Application::input() {
    return *input_;
}

FileSystem& Application::fileSystem() {
    return *fileSystem_;
}

Clock& Application::clock() {
    return *clock_;
}

SceneManager& Application::sceneManager() {
    return sceneManager_;
}

BookLibrary& Application::library() {
    return library_;
}

BookCache& Application::bookCache() {
    return bookCache_;
}

ProgressStore& Application::progressStore() {
    return progressStore_;
}

SettingsStore& Application::settingsStore() {
    return settingsStore_;
}

TextBlipPlayer& Application::textBlipPlayer() {
    return textBlipPlayer_;
}

const ReaderSettings& Application::settings() const {
    return settings_;
}

ReaderSettings& Application::settings() {
    return settings_;
}

std::string Application::buildScreenshotPath() const {
    if (!fileSystem_) {
        return {};
    }

    const std::string screenshotsDir = fileSystem_->savesPath() + "/Screenshots";
    if (!const_cast<FileSystem&>(*fileSystem_).createDirectories(screenshotsDir)) {
        return {};
    }

    const auto now = std::chrono::system_clock::now();
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::ostringstream path;
    path << screenshotsDir << "/RetroRead-" << millis << ".bmp";
    return path.str();
}
