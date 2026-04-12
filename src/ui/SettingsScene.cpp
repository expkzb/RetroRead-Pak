#include "ui/SettingsScene.h"

#include <memory>
#include <utility>

#include "app/Application.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "ui/ReaderScene.h"
#include "ui/ThemePalette.h"

namespace {
const char* themePresetName(ThemePreset preset) {
    switch (preset) {
    case ThemePreset::BoldAmber:
        return "Bold Amber";
    case ThemePreset::CalmTeal:
        return "Calm Teal";
    case ThemePreset::FrameBlue:
        return "Frame Blue";
    case ThemePreset::BattleRed:
        return "Battle Red";
    case ThemePreset::MintLcd:
        return "Mint LCD";
    case ThemePreset::GbInvert:
        return "GB Invert";
    case ThemePreset::ClassicDark:
    default:
        return "Classic Dark";
    }
}

const char* fontPresetName(FontPreset preset) {
    switch (preset) {
    case FontPreset::Pixel:
        return "Pixel";
    case FontPreset::Normal:
    default:
        return "Normal";
    }
}

const char* textVoiceModeName(TextVoiceMode mode) {
    switch (mode) {
    case TextVoiceMode::Fixed:
        return "Fixed";
    case TextVoiceMode::FollowText:
        return "Follow Text";
    case TextVoiceMode::Off:
    default:
        return "Off";
    }
}

const char* performanceModeName(PerformanceMode mode) {
    switch (mode) {
    case PerformanceMode::Hud:
        return "HUD";
    case PerformanceMode::Log:
        return "Log";
    case PerformanceMode::Off:
    default:
        return "Off";
    }
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
}

SettingsScene::SettingsScene(Application& app, BookScript book, ReadingProgress progress)
    : AppScene(app), book_(std::move(book)), progress_(std::move(progress)) {
}

void SettingsScene::update(float dt) {
    (void)dt;
    Input& input = app_.input();

    if (input.wasPressed(Action::Back) || input.wasPressed(Action::Start) || input.wasPressed(Action::OpenMenu)) {
        returnToReader();
        return;
    }

    if (input.wasPressed(Action::Up) && selectedIndex_ > 0) {
        --selectedIndex_;
        clampScroll();
    }

    if (input.wasPressed(Action::Down) && selectedIndex_ < 5) {
        ++selectedIndex_;
        clampScroll();
    }

    if (input.wasPressed(Action::Left)) {
        applyDelta(-1);
    }

    if (input.wasPressed(Action::Right) || input.wasPressed(Action::Confirm)) {
        applyDelta(1);
    }
}

void SettingsScene::render(Renderer& renderer) {
    const ReaderSettings& settings = app_.settings();
    const ThemePalette palette = themePalette(settings.themePreset);

    renderer.clear(palette.screenBackground);

    renderer.drawText(
        "Reader Settings",
        Rect{60, 18, 860, uiSpacing(40, 70)},
        palette.headerText,
        uiFont(28, 42),
        TextAlign::Left,
        settings.fontPreset);

    renderer.drawText(
        "Up/Down: select  Left/Right: change  Start/Menu: back",
        Rect{60, 92, 920, uiSpacing(24, 32)},
        palette.secondaryText,
        uiFont(16, 22),
        TextAlign::Left,
        settings.fontPreset);

    if (settings.performanceMode == PerformanceMode::Hud) {
        renderer.drawText(
            app_.performanceHudText(),
            Rect{renderer.screenWidth() - 360, 18, 320, 24},
            palette.secondaryText,
            uiFont(14, 18),
            TextAlign::Right,
            settings.fontPreset);
    }

    const std::string rows[6] = {
        "Reader Font Size: " + std::to_string(settings.fontSize),
        "Text Speed: " + std::to_string(settings.textSpeed) + " ms",
        std::string("Text Voice: ") + textVoiceModeName(settings.textVoiceMode),
        std::string("Theme: ") + themePresetName(settings.themePreset),
        std::string("Font: ") + fontPresetName(settings.fontPreset),
        std::string("Performance: ") + performanceModeName(settings.performanceMode),
    };

    int y = uiSpacing(170, 200);
    const int startIndex = scrollOffset_;
    const int endIndex = std::min(6, startIndex + 5);
    for (int i = startIndex; i < endIndex; ++i) {
        const bool selected = i == selectedIndex_;
        const Rect rowRect{72, y - uiSpacing(18, 26), 884, uiSpacing(62, 98)};
        if (selected) {
            renderer.fillRect(rowRect, palette.selectionFill);
            renderer.drawRect(rowRect, palette.selectionOutline);
        }
        const int textTop = rowRect.y + (rowRect.h - uiSpacing(30, 52)) / 2;
        renderer.drawText(
            rows[i],
            Rect{88, textTop, 860, uiSpacing(30, 52)},
            selected ? palette.selectionText : palette.primaryText,
            uiFont(22, 40),
            TextAlign::Left,
            settings.fontPreset);
        y += uiSpacing(56, 92);
    }
}

void SettingsScene::applyDelta(int delta) {
    ReaderSettings& settings = app_.settings();
    switch (selectedIndex_) {
    case 0: {
        int value = static_cast<int>(settings.fontSize) + delta * 2;
        settings.fontSize = static_cast<std::uint32_t>(std::max(16, std::min(40, value)));
        break;
    }
    case 1: {
        int value = static_cast<int>(settings.textSpeed) + delta * 5;
        settings.textSpeed = static_cast<std::uint32_t>(std::max(5, std::min(120, value)));
        progress_.textSpeed = settings.textSpeed;
        break;
    }
    case 2: {
        int mode = static_cast<int>(settings.textVoiceMode);
        constexpr int kModeCount = 3;
        mode = (mode + delta + kModeCount) % kModeCount;
        settings.textVoiceMode = static_cast<TextVoiceMode>(mode);
        break;
    }
    case 3: {
        int preset = static_cast<int>(settings.themePreset);
        constexpr int kPresetCount = 7;
        preset = (preset + delta + kPresetCount) % kPresetCount;
        settings.themePreset = static_cast<ThemePreset>(preset);
        break;
    }
    case 4: {
        int preset = static_cast<int>(settings.fontPreset);
        preset = (preset + delta + 2) % 2;
        settings.fontPreset = static_cast<FontPreset>(preset);
        break;
    }
    case 5:
        if (delta != 0) {
            int mode = static_cast<int>(settings.performanceMode);
            constexpr int kModeCount = 3;
            mode = (mode + delta + kModeCount) % kModeCount;
            settings.performanceMode = static_cast<PerformanceMode>(mode);
        }
        break;
    default:
        break;
    }

    app_.settingsStore().save(app_.fileSystem(), settings);
}

void SettingsScene::returnToReader() {
    app_.sceneManager().replace(std::make_unique<ReaderScene>(app_, std::move(book_), progress_));
}

void SettingsScene::clampScroll() {
    const int visibleRows = 5;
    const int maxOffset = std::max(0, 6 - visibleRows);
    if (selectedIndex_ < scrollOffset_) {
        scrollOffset_ = selectedIndex_;
    }
    if (selectedIndex_ >= scrollOffset_ + visibleRows) {
        scrollOffset_ = selectedIndex_ - visibleRows + 1;
    }
    scrollOffset_ = std::max(0, std::min(scrollOffset_, maxOffset));
}
