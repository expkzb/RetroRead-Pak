#pragma once

enum class Action {
    Confirm,
    FastForward,
    ToggleAuto,
    OpenMenu,
    Start,
    Back,
    Secondary,
    Up,
    Down,
    Left,
    Right,
    PrevChapter,
    NextChapter,
};

class Input {
public:
    virtual ~Input() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void poll() = 0;

    virtual bool wasPressed(Action action) const = 0;
    virtual bool isHeld(Action action) const = 0;
    virtual bool quitRequested() const = 0;
};
