#include "platform/sdl/SDLClock.h"

float SDLClock::tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration<float>(now - previous_).count();
    previous_ = now;
    return delta;
}
