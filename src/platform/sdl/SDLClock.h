#pragma once

#include <chrono>

#include "platform/Clock.h"

class SDLClock final : public Clock {
public:
    float tick() override;

private:
    std::chrono::steady_clock::time_point previous_ = std::chrono::steady_clock::now();
};
