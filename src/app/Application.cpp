#include "app/Application.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <thread>
#include <memory>
#include <sstream>
#include <utility>

#include "ui/BookListScene.h"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace {
std::uint64_t currentTimeMs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

std::uint64_t readProcessJiffies() {
#ifdef _WIN32
    return 0;
#else
    std::ifstream stat("/proc/self/stat");
    if (!stat.is_open()) {
        return 0;
    }

    std::string line;
    std::getline(stat, line);
    const std::size_t closingParen = line.rfind(')');
    if (closingParen == std::string::npos || closingParen + 2 >= line.size()) {
        return 0;
    }

    std::istringstream rest(line.substr(closingParen + 2));
    std::string token;
    std::uint64_t utime = 0;
    std::uint64_t stime = 0;
    for (int index = 3; rest >> token; ++index) {
        if (index == 14) {
            utime = std::stoull(token);
        } else if (index == 15) {
            stime = std::stoull(token);
            break;
        }
    }

    return utime + stime;
#endif
}
}

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
    constexpr float kIdleSleepSeconds = 1.0f / 30.0f;
    float idleSeconds = 0.0f;
    bool forceRender = true;

    while (running_) {
        const float dt = clock_->tick();
        updatePerformanceStats(dt);

        input_->poll();
        if (input_->quitRequested()) {
            requestQuit();
        }

        sceneManager_.update(dt);

        const bool hasInput =
            input_->wasPressed(Action::Confirm) || input_->wasPressed(Action::FastForward) ||
            input_->wasPressed(Action::ToggleAuto) || input_->wasPressed(Action::Screenshot) ||
            input_->wasPressed(Action::OpenMenu) || input_->wasPressed(Action::Start) ||
            input_->wasPressed(Action::Back) || input_->wasPressed(Action::Secondary) ||
            input_->wasPressed(Action::Up) || input_->wasPressed(Action::Down) ||
            input_->wasPressed(Action::Left) || input_->wasPressed(Action::Right) ||
            input_->wasPressed(Action::PrevChapter) || input_->wasPressed(Action::NextChapter);

        const bool shouldRender =
            forceRender || hasInput || sceneManager_.shouldRenderContinuously() || sceneManager_.consumeRenderRequest();
        if (shouldRender) {
            renderer_->beginFrame();
            sceneManager_.render(*renderer_);
            if (input_->wasPressed(Action::Screenshot)) {
                const std::string screenshotPath = buildScreenshotPath();
                if (!screenshotPath.empty()) {
                    renderer_->saveScreenshot(screenshotPath);
                }
            }
            renderer_->endFrame();
            forceRender = false;
        }

        if (hasInput) {
            idleSeconds = 0.0f;
            continue;
        }

        idleSeconds += dt;
        if (idleSeconds >= 0.25f) {
            std::this_thread::sleep_for(std::chrono::duration<float>(kIdleSleepSeconds));
        }
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

void Application::updatePerformanceStats(float dt) {
    const float frameMs = dt * 1000.0f;
    averageFrameMs_ = averageFrameMs_ <= 0.0f ? frameMs : averageFrameMs_ * 0.92f + frameMs * 0.08f;

    const float fps = dt > 0.0001f ? 1.0f / dt : 0.0f;
    averageFps_ = averageFps_ <= 0.0f ? fps : averageFps_ * 0.92f + fps * 0.08f;

    cpuSampleTimer_ += dt;
    if (cpuSampleTimer_ >= 0.5f) {
        cpuSampleTimer_ = 0.0f;
        sampleCpuUsage();
    }

    if (settings_.performanceMode == PerformanceMode::Log) {
        perfLogTimer_ += dt;
        if (perfLogTimer_ >= 0.2f) {
            perfLogTimer_ = 0.0f;
            appendPerformanceLogLine();
        }
    } else {
        perfLogTimer_ = 0.0f;
    }
}

void Application::sampleCpuUsage() {
    const std::uint64_t nowMs = currentTimeMs();
    const std::uint64_t processJiffies = readProcessJiffies();
    if (processJiffies == 0 || lastCpuSampleMs_ == 0 || lastProcessJiffies_ == 0) {
        lastCpuSampleMs_ = nowMs;
        lastProcessJiffies_ = processJiffies;
        return;
    }

#ifndef _WIN32
    const std::uint64_t deltaMs = nowMs > lastCpuSampleMs_ ? nowMs - lastCpuSampleMs_ : 0;
    const std::uint64_t deltaJiffies =
        processJiffies >= lastProcessJiffies_ ? processJiffies - lastProcessJiffies_ : 0;
    if (deltaMs > 0) {
        const long hz = sysconf(_SC_CLK_TCK);
        if (hz > 0) {
            sampledCpuPercent_ =
                std::min(999.0f, static_cast<float>(deltaJiffies) * 100000.0f / (static_cast<float>(hz) * deltaMs));
        }
    }
#endif

    lastCpuSampleMs_ = nowMs;
    lastProcessJiffies_ = processJiffies;
}

std::string Application::performanceHudText() const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(0) << averageFps_ << " FPS  ";
    out << std::setprecision(1) << averageFrameMs_ << " ms";
    if (sampledCpuPercent_ >= 0.0f) {
        out << "  CPU " << std::setprecision(1) << sampledCpuPercent_ << "%";
    }
    return out.str();
}

std::string Application::performanceLogPath() const {
    if (!fileSystem_) {
        return {};
    }
    return fileSystem_->savesPath() + "/perf.log";
}

void Application::appendPerformanceLogLine() const {
    if (!fileSystem_) {
        return;
    }

    const std::string path = performanceLogPath();
    std::ifstream in(path);
    std::string content;
    if (in.is_open()) {
        std::ostringstream existing;
        existing << in.rdbuf();
        content = existing.str();
    }

    std::ostringstream line;
    line << "fps=" << std::fixed << std::setprecision(1) << averageFps_
         << "\tframe_ms=" << std::setprecision(2) << averageFrameMs_;
    if (sampledCpuPercent_ >= 0.0f) {
        line << "\tcpu=" << std::setprecision(1) << sampledCpuPercent_;
    }
    line << '\n';

    fileSystem_->writeTextFile(path, content + line.str());
}
