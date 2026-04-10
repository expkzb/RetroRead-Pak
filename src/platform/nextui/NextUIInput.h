#pragma once

#include <array>

#include "platform/Input.h"

enum class NextUIButton {
    A,
    B,
    X,
    Y,
    L1,
    R1,
    Start,
    Select,
    Up,
    Down,
    Left,
    Right,
    Count,
};

struct NextUIInputHooks {
    void* userdata = nullptr;
    bool (*initialize)(void* userdata) = nullptr;
    void (*shutdown)(void* userdata) = nullptr;
    void (*pollButtons)(void* userdata, bool* buttons, std::size_t buttonCount, bool* quitRequested) = nullptr;
};

class NextUIInput final : public Input {
public:
    static void registerHooks(const NextUIInputHooks& hooks);

    bool initialize() override;
    void shutdown() override;
    void poll() override;

    bool wasPressed(Action action) const override;
    bool isHeld(Action action) const override;
    bool quitRequested() const override;

    void beginFrame();
    void setButtonDown(NextUIButton button);
    void setButtonUp(NextUIButton button);
    void setQuitRequested(bool requested);

private:
    static std::size_t actionIndex(Action action);
    static Action mapButton(NextUIButton button);
    static std::size_t buttonIndex(NextUIButton button);

    void setPressed(Action action);
    void setHeld(Action action, bool held);

private:
    std::array<bool, static_cast<std::size_t>(Action::NextChapter) + 1> pressed_{};
    std::array<bool, static_cast<std::size_t>(Action::NextChapter) + 1> held_{};
    std::array<bool, static_cast<std::size_t>(NextUIButton::Count)> deviceHeld_{};
    bool quitRequested_ = false;
};
