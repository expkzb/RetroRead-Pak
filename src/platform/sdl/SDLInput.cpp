#include "platform/sdl/SDLInput.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
void triggerScreenshotCombo(std::array<bool, 14>& pressed, std::array<bool, 14>& held) {
    const std::size_t startIndex = static_cast<std::size_t>(Action::Start);
    const std::size_t secondaryIndex = static_cast<std::size_t>(Action::Secondary);
    const std::size_t screenshotIndex = static_cast<std::size_t>(Action::Screenshot);

    if (!held[startIndex] || !held[secondaryIndex]) {
        return;
    }

    pressed[startIndex] = false;
    pressed[secondaryIndex] = false;
    held[startIndex] = false;
    held[secondaryIndex] = false;
    pressed[screenshotIndex] = true;
}
}

SDLInput::SDLInput() = default;

bool SDLInput::initialize() {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        return false;
    }
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    if (const char* value = std::getenv("NEXTREADING_INPUT_MODE")) {
        preferJoystick_ = std::string(value) == "joystick";
    }
    if (const char* value = std::getenv("NEXTREADING_INPUT_DEBUG")) {
        debugLogging_ = std::atoi(value) != 0;
    }
    if (const char* value = std::getenv("NEXTREADING_BTN_A")) buttonA_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_B")) buttonB_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_X")) buttonX_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_Y")) buttonY_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_L1")) buttonL1_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_R1")) buttonR1_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_SELECT")) buttonSelect_ = std::atoi(value);
    if (const char* value = std::getenv("NEXTREADING_BTN_START")) buttonStart_ = std::atoi(value);

    if (SDL_NumJoysticks() > 0) {
        if (!preferJoystick_ && SDL_IsGameController(0) == SDL_TRUE) {
            controller_ = SDL_GameControllerOpen(0);
        }
        if (controller_ == nullptr) {
            joystick_ = SDL_JoystickOpen(0);
        }
    }
    return true;
}

void SDLInput::shutdown() {
    if (controller_ != nullptr) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }
    if (joystick_ != nullptr) {
        SDL_JoystickClose(joystick_);
        joystick_ = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
}

void SDLInput::poll() {
    pressed_.fill(false);

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            quitRequested_ = true;
            break;
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                handleKeyDown(event.key.keysym.sym);
            }
            break;
        case SDL_KEYUP:
            handleKeyUp(event.key.keysym.sym);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            handleControllerButtonDown(event.cbutton.button);
            break;
        case SDL_CONTROLLERBUTTONUP:
            handleControllerButtonUp(event.cbutton.button);
            break;
        case SDL_JOYBUTTONDOWN:
            handleJoystickButtonDown(event.jbutton.button);
            break;
        case SDL_JOYBUTTONUP:
            handleJoystickButtonUp(event.jbutton.button);
            break;
        case SDL_JOYHATMOTION:
            handleJoystickHat(event.jhat.value);
            break;
        default:
            break;
        }
    }
}

bool SDLInput::wasPressed(Action action) const {
    return pressed_[indexFor(action)];
}

bool SDLInput::isHeld(Action action) const {
    return held_[indexFor(action)];
}

bool SDLInput::quitRequested() const {
    return quitRequested_;
}

std::size_t SDLInput::indexFor(Action action) {
    return static_cast<std::size_t>(action);
}

const char* SDLInput::actionName(Action action) const {
    switch (action) {
    case Action::Confirm:
        return "Confirm";
    case Action::FastForward:
        return "FastForward";
    case Action::ToggleAuto:
        return "ToggleAuto";
    case Action::Screenshot:
        return "Screenshot";
    case Action::OpenMenu:
        return "OpenMenu";
    case Action::Start:
        return "Start";
    case Action::Back:
        return "Back";
    case Action::Secondary:
        return "Secondary";
    case Action::Up:
        return "Up";
    case Action::Down:
        return "Down";
    case Action::Left:
        return "Left";
    case Action::Right:
        return "Right";
    case Action::PrevChapter:
        return "PrevChapter";
    case Action::NextChapter:
        return "NextChapter";
    }

    return "Unknown";
}

void SDLInput::logPressed(Action action, const char* source, int code) const {
    if (!debugLogging_) {
        return;
    }
    std::fprintf(stderr, "NEXTREADING_INPUT %s code=%d action=%s\n", source, code, actionName(action));
    std::fflush(stderr);
}

void SDLInput::setPressed(Action action) {
    pressed_[indexFor(action)] = true;
}

void SDLInput::setHeld(Action action, bool held) {
    held_[indexFor(action)] = held;
    applyQuitCombo(action);
}

void SDLInput::applyQuitCombo(Action action) {
    if (action == Action::Start || action == Action::Back) {
        if (held_[indexFor(Action::Start)] && held_[indexFor(Action::Back)]) {
            quitRequested_ = true;
        }
    }
}

void SDLInput::handleKeyDown(SDL_Keycode key) {
    switch (key) {
    case SDLK_RETURN:
    case SDLK_z:
    case SDLK_a:
        setPressed(Action::Confirm);
        setHeld(Action::Confirm, true);
        logPressed(Action::Confirm, "keyboard", static_cast<int>(key));
        break;
    case SDLK_x:
    case SDLK_SPACE:
        setPressed(Action::FastForward);
        setHeld(Action::FastForward, true);
        logPressed(Action::FastForward, "keyboard", static_cast<int>(key));
        break;
    case SDLK_t:
    case SDLK_TAB:
        setPressed(Action::ToggleAuto);
        setHeld(Action::ToggleAuto, true);
        logPressed(Action::ToggleAuto, "keyboard", static_cast<int>(key));
        break;
    case SDLK_F12:
        setPressed(Action::Screenshot);
        setHeld(Action::Screenshot, true);
        logPressed(Action::Screenshot, "keyboard", static_cast<int>(key));
        break;
    case SDLK_p:
    case SDLK_F1:
        setPressed(Action::Start);
        setHeld(Action::Start, true);
        logPressed(Action::Start, "keyboard", static_cast<int>(key));
        triggerScreenshotCombo(pressed_, held_);
        break;
    case SDLK_ESCAPE:
        setPressed(Action::OpenMenu);
        setHeld(Action::OpenMenu, true);
        logPressed(Action::OpenMenu, "keyboard", static_cast<int>(key));
        break;
    case SDLK_BACKSPACE:
        setPressed(Action::Back);
        setHeld(Action::Back, true);
        logPressed(Action::Back, "keyboard", static_cast<int>(key));
        break;
    case SDLK_c:
    case SDLK_s:
        setPressed(Action::Secondary);
        setHeld(Action::Secondary, true);
        logPressed(Action::Secondary, "keyboard", static_cast<int>(key));
        triggerScreenshotCombo(pressed_, held_);
        break;
    case SDLK_UP:
        setPressed(Action::Up);
        setHeld(Action::Up, true);
        logPressed(Action::Up, "keyboard", static_cast<int>(key));
        break;
    case SDLK_DOWN:
        setPressed(Action::Down);
        setHeld(Action::Down, true);
        logPressed(Action::Down, "keyboard", static_cast<int>(key));
        break;
    case SDLK_LEFT:
        setPressed(Action::Left);
        setHeld(Action::Left, true);
        logPressed(Action::Left, "keyboard", static_cast<int>(key));
        break;
    case SDLK_RIGHT:
        setPressed(Action::Right);
        setHeld(Action::Right, true);
        logPressed(Action::Right, "keyboard", static_cast<int>(key));
        break;
    case SDLK_q:
    case SDLK_LEFTBRACKET:
        setPressed(Action::PrevChapter);
        setHeld(Action::PrevChapter, true);
        logPressed(Action::PrevChapter, "keyboard", static_cast<int>(key));
        break;
    case SDLK_e:
    case SDLK_RIGHTBRACKET:
        setPressed(Action::NextChapter);
        setHeld(Action::NextChapter, true);
        logPressed(Action::NextChapter, "keyboard", static_cast<int>(key));
        break;
    default:
        break;
    }
}

void SDLInput::handleKeyUp(SDL_Keycode key) {
    switch (key) {
    case SDLK_RETURN:
    case SDLK_z:
    case SDLK_a:
        setHeld(Action::Confirm, false);
        break;
    case SDLK_x:
    case SDLK_SPACE:
        setHeld(Action::FastForward, false);
        break;
    case SDLK_t:
    case SDLK_TAB:
        setHeld(Action::ToggleAuto, false);
        break;
    case SDLK_F12:
        setHeld(Action::Screenshot, false);
        break;
    case SDLK_p:
    case SDLK_F1:
        setHeld(Action::Start, false);
        break;
    case SDLK_ESCAPE:
        setHeld(Action::OpenMenu, false);
        break;
    case SDLK_BACKSPACE:
        setHeld(Action::Back, false);
        break;
    case SDLK_c:
    case SDLK_s:
        setHeld(Action::Secondary, false);
        break;
    case SDLK_UP:
        setHeld(Action::Up, false);
        break;
    case SDLK_DOWN:
        setHeld(Action::Down, false);
        break;
    case SDLK_LEFT:
        setHeld(Action::Left, false);
        break;
    case SDLK_RIGHT:
        setHeld(Action::Right, false);
        break;
    case SDLK_q:
    case SDLK_LEFTBRACKET:
        setHeld(Action::PrevChapter, false);
        break;
    case SDLK_e:
    case SDLK_RIGHTBRACKET:
        setHeld(Action::NextChapter, false);
        break;
    default:
        break;
    }
}

void SDLInput::handleControllerButtonDown(Uint8 button) {
    if (preferJoystick_) {
        return;
    }

    switch (button) {
    case SDL_CONTROLLER_BUTTON_A:
        setPressed(Action::Confirm);
        setHeld(Action::Confirm, true);
        logPressed(Action::Confirm, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_B:
        setPressed(Action::FastForward);
        setHeld(Action::FastForward, true);
        logPressed(Action::FastForward, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_X:
        setPressed(Action::ToggleAuto);
        setHeld(Action::ToggleAuto, true);
        logPressed(Action::ToggleAuto, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_Y:
        setPressed(Action::Secondary);
        setHeld(Action::Secondary, true);
        logPressed(Action::Secondary, "controller", button);
        triggerScreenshotCombo(pressed_, held_);
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        setPressed(Action::PrevChapter);
        setHeld(Action::PrevChapter, true);
        logPressed(Action::PrevChapter, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        setPressed(Action::NextChapter);
        setHeld(Action::NextChapter, true);
        logPressed(Action::NextChapter, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_START:
        setPressed(Action::Start);
        setHeld(Action::Start, true);
        logPressed(Action::Start, "controller", button);
        triggerScreenshotCombo(pressed_, held_);
        break;
    case SDL_CONTROLLER_BUTTON_BACK:
    case SDL_CONTROLLER_BUTTON_GUIDE:
        setPressed(Action::Back);
        setHeld(Action::Back, true);
        setPressed(Action::OpenMenu);
        setHeld(Action::OpenMenu, true);
        logPressed(Action::Back, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        setPressed(Action::Up);
        setHeld(Action::Up, true);
        logPressed(Action::Up, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        setPressed(Action::Down);
        setHeld(Action::Down, true);
        logPressed(Action::Down, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        setPressed(Action::Left);
        setHeld(Action::Left, true);
        logPressed(Action::Left, "controller", button);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        setPressed(Action::Right);
        setHeld(Action::Right, true);
        logPressed(Action::Right, "controller", button);
        break;
    default:
        break;
    }
}

void SDLInput::handleControllerButtonUp(Uint8 button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_A:
        setHeld(Action::Confirm, false);
        break;
    case SDL_CONTROLLER_BUTTON_B:
        setHeld(Action::FastForward, false);
        break;
    case SDL_CONTROLLER_BUTTON_X:
        setHeld(Action::ToggleAuto, false);
        break;
    case SDL_CONTROLLER_BUTTON_Y:
        setHeld(Action::Secondary, false);
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        setHeld(Action::PrevChapter, false);
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        setHeld(Action::NextChapter, false);
        break;
    case SDL_CONTROLLER_BUTTON_START:
        setHeld(Action::Start, false);
        break;
    case SDL_CONTROLLER_BUTTON_BACK:
    case SDL_CONTROLLER_BUTTON_GUIDE:
        setHeld(Action::Back, false);
        setHeld(Action::OpenMenu, false);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        setHeld(Action::Up, false);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        setHeld(Action::Down, false);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        setHeld(Action::Left, false);
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        setHeld(Action::Right, false);
        break;
    default:
        break;
    }
}

void SDLInput::handleJoystickButtonDown(Uint8 button) {
    Action action = Action::Back;
    if (!tryMapJoystickButton(button, action)) {
        if (debugLogging_) {
            std::fprintf(stderr, "NEXTREADING_INPUT joystick code=%u action=Unmapped\n", button);
            std::fflush(stderr);
        }
        return;
    }
    setPressed(action);
    setHeld(action, true);
    logPressed(action, "joystick", button);

    // On handheld mappings, Select often doubles as back/menu.
    if (action == Action::Back) {
        setPressed(Action::OpenMenu);
        setHeld(Action::OpenMenu, true);
    }
    if (action == Action::Start || action == Action::Secondary) {
        triggerScreenshotCombo(pressed_, held_);
    }
}

void SDLInput::handleJoystickButtonUp(Uint8 button) {
    Action action = Action::Back;
    if (!tryMapJoystickButton(button, action)) {
        return;
    }
    setHeld(action, false);
    if (action == Action::Back) {
        setHeld(Action::OpenMenu, false);
    }
}

void SDLInput::handleJoystickHat(Uint8 value) {
    setHeld(Action::Up, (value & SDL_HAT_UP) != 0);
    setHeld(Action::Down, (value & SDL_HAT_DOWN) != 0);
    setHeld(Action::Left, (value & SDL_HAT_LEFT) != 0);
    setHeld(Action::Right, (value & SDL_HAT_RIGHT) != 0);

    if ((value & SDL_HAT_UP) != 0) setPressed(Action::Up);
    if ((value & SDL_HAT_DOWN) != 0) setPressed(Action::Down);
    if ((value & SDL_HAT_LEFT) != 0) setPressed(Action::Left);
    if ((value & SDL_HAT_RIGHT) != 0) setPressed(Action::Right);
}

bool SDLInput::tryMapJoystickButton(Uint8 button, Action& action) const {
    if (button == static_cast<Uint8>(buttonA_)) {
        action = Action::Confirm;
        return true;
    }
    if (button == static_cast<Uint8>(buttonB_)) {
        action = Action::FastForward;
        return true;
    }
    if (button == static_cast<Uint8>(buttonX_)) {
        action = Action::ToggleAuto;
        return true;
    }
    if (button == static_cast<Uint8>(buttonY_)) {
        action = Action::Secondary;
        return true;
    }
    if (button == static_cast<Uint8>(buttonL1_)) {
        action = Action::PrevChapter;
        return true;
    }
    if (button == static_cast<Uint8>(buttonR1_)) {
        action = Action::NextChapter;
        return true;
    }
    if (button == static_cast<Uint8>(buttonStart_)) {
        action = Action::Start;
        return true;
    }
    if (button == static_cast<Uint8>(buttonSelect_)) {
        action = Action::Back;
        return true;
    }
    return false;
}
