#pragma once

#include "platform/Clock.h"

class NextUIClock final : public Clock {
public:
    float tick() override;
};
