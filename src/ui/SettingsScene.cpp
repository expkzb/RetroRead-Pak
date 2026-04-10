#include "ui/SettingsScene.h"

#include <memory>
#include <utility>

#include "app/Application.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "ui/ReaderScene.h"

namespace {
const char* dialogueStyleName(DialogueStyle style) {
    switch (style) {
    case DialogueStyle::Frame:
        return "Frame";
    case DialogueStyle::Battle:
        return "Battle";
    case DialogueStyle::Bold:
        return "Bold";
    case DialogueStyle::Calm:
        return "Calm";
    case DialogueStyle::Classic:
    default:
        return "Classic";
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
    }

    if (input.wasPressed(Action::Down) && selectedIndex_ < 4) {
        ++selectedIndex_;
    }

    if (input.wasPressed(Action::Left)) {
        applyDelta(-1);
    }

    if (input.wasPressed(Action::Right) || input.wasPressed(Action::Confirm)) {
        applyDelta(1);
    }
}

void SettingsScene::render(Renderer& renderer) {
    renderer.clear(Color{8, 12, 18, 255});
    const ReaderSettings& settings = app_.settings();

    renderer.drawText(
        "Reader Settings",
        Rect{60, 18, 860, uiSpacing(40, 70)},
        Color{255, 233, 188, 255},
        uiFont(28, 42),
        TextAlign::Left,
        settings.fontPreset);

    renderer.drawText(
        "Up/Down: select  Left/Right: change  Start/Menu: back",
        Rect{60, 92, 920, uiSpacing(24, 32)},
        Color{174, 194, 214, 255},
        uiFont(16, 22),
        TextAlign::Left,
        settings.fontPreset);

    const std::string rows[5] = {
        "Reader Font Size: " + std::to_string(settings.fontSize),
        "Text Speed: " + std::to_string(settings.textSpeed) + " ms",
        std::string("Text Voice: ") + textVoiceModeName(settings.textVoiceMode),
        std::string("Dialogue Style: ") + dialogueStyleName(settings.dialogueStyle),
        std::string("Font: ") + fontPresetName(settings.fontPreset),
    };

    int y = uiSpacing(170, 200);
    for (int i = 0; i < 5; ++i) {
        const bool selected = i == selectedIndex_;
        renderer.drawText(
            rows[i],
            Rect{88, y, 860, uiSpacing(30, 52)},
            selected ? Color{255, 233, 188, 255} : Color{230, 236, 244, 255},
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
        int style = static_cast<int>(settings.dialogueStyle);
        constexpr int kStyleCount = 5;
        style = (style + delta + kStyleCount) % kStyleCount;
        settings.dialogueStyle = static_cast<DialogueStyle>(style);
        break;
    }
    case 4: {
        int preset = static_cast<int>(settings.fontPreset);
        preset = (preset + delta + 2) % 2;
        settings.fontPreset = static_cast<FontPreset>(preset);
        break;
    }
    default:
        break;
    }

    app_.settingsStore().save(app_.fileSystem(), settings);
}

void SettingsScene::returnToReader() {
    app_.sceneManager().replace(std::make_unique<ReaderScene>(app_, std::move(book_), progress_));
}
