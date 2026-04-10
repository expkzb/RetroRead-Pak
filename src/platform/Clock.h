#pragma once

class Clock {
public:
    virtual ~Clock() = default;
    virtual float tick() = 0;
};
