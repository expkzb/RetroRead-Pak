#include "platform/nextui/NextUIInput.h"

namespace {
NextUIInputHooks g_hooks;
}

void NextUIInput::registerHooks(const NextUIInputHooks& hooks) {
    g_hooks = hooks;
}

bool NextUIInput::initialize() {
    if (g_hooks.initialize != nullptr) {
        return g_hooks.initialize(g_hooks.userdata);
    }

    return true;
}

void NextUIInput::shutdown() {
    if (g_hooks.shutdown != nullptr) {
        g_hooks.shutdown(g_hooks.userdata);
    }
}

void NextUIInput::poll() {
    beginFrame();

    if (g_hooks.pollButtons == nullptr) {
        return;
    }

    std::array<bool, static_cast<std::size_t>(NextUIButton::Count)> nextButtons{};
    bool nextQuitRequested = quitRequested_;
    g_hooks.pollButtons(
        g_hooks.userdata,
        nextButtons.data(),
        nextButtons.size(),
        &nextQuitRequested);

    quitRequested_ = nextQuitRequested;
    for (std::size_t i = 0; i < nextButtons.size(); ++i) {
        const bool wasHeld = deviceHeld_[i];
        const bool isHeldNow = nextButtons[i];
        deviceHeld_[i] = isHeldNow;

        if (isHeldNow && !wasHeld) {
            setButtonDown(static_cast<NextUIButton>(i));
        } else if (!isHeldNow && wasHeld) {
            setButtonUp(static_cast<NextUIButton>(i));
        }
    }
}

bool NextUIInput::wasPressed(Action action) const {
    return pressed_[actionIndex(action)];
}

bool NextUIInput::isHeld(Action action) const {
    return held_[actionIndex(action)];
}

bool NextUIInput::quitRequested() const {
    return quitRequested_;
}

void NextUIInput::beginFrame() {
    pressed_.fill(false);
}

void NextUIInput::setButtonDown(NextUIButton button) {
    const Action action = mapButton(button);
    setPressed(action);
    setHeld(action, true);
}

void NextUIInput::setButtonUp(NextUIButton button) {
    setHeld(mapButton(button), false);
}

void NextUIInput::setQuitRequested(bool requested) {
    quitRequested_ = requested;
}

std::size_t NextUIInput::actionIndex(Action action) {
    return static_cast<std::size_t>(action);
}

std::size_t NextUIInput::buttonIndex(NextUIButton button) {
    return static_cast<std::size_t>(button);
}

Action NextUIInput::mapButton(NextUIButton button) {
    switch (button) {
    case NextUIButton::A:
        return Action::Confirm;
    case NextUIButton::B:
        return Action::FastForward;
    case NextUIButton::X:
        return Action::ToggleAuto;
    case NextUIButton::Y:
        return Action::Secondary;
    case NextUIButton::L1:
        return Action::PrevChapter;
    case NextUIButton::R1:
        return Action::NextChapter;
    case NextUIButton::Start:
        return Action::Start;
    case NextUIButton::Select:
        return Action::Back;
    case NextUIButton::Up:
        return Action::Up;
    case NextUIButton::Down:
        return Action::Down;
    case NextUIButton::Left:
        return Action::Left;
    case NextUIButton::Right:
        return Action::Right;
    case NextUIButton::Count:
    default:
        return Action::Back;
    }
}

void NextUIInput::setPressed(Action action) {
    pressed_[actionIndex(action)] = true;
}

void NextUIInput::setHeld(Action action, bool held) {
    held_[actionIndex(action)] = held;
}
