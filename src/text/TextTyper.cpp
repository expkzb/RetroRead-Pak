#include "text/TextTyper.h"

#include <algorithm>

#include "text/Utf8.h"

void TextTyper::start(const std::string& text, int baseSpeedMs) {
    text_ = text;
    codepoints_ = utf8::splitCodepoints(text_);
    visibleChars_ = 0;
    accumulatedMs_ = 0.0f;
    baseSpeedMs_ = std::max(baseSpeedMs, 1);
    skipNextUpdate_ = true;
}

int TextTyper::update(float dtSeconds) {
    if (isComplete()) {
        return 0;
    }

    if (skipNextUpdate_) {
        skipNextUpdate_ = false;
        return 0;
    }

    constexpr int kMaxCharsPerUpdate = 2;
    const float clampedDtMs =
        std::min(dtSeconds * 1000.0f, static_cast<float>(std::max(baseSpeedMs_ * kMaxCharsPerUpdate, baseSpeedMs_)));
    accumulatedMs_ += clampedDtMs;
    int advanced = 0;
    while (accumulatedMs_ >= static_cast<float>(baseSpeedMs_) && !isComplete() && advanced < kMaxCharsPerUpdate) {
        accumulatedMs_ -= static_cast<float>(baseSpeedMs_);
        ++visibleChars_;
        ++advanced;
    }
    return advanced;
}

void TextTyper::revealAll() {
    visibleChars_ = codepoints_.size();
    accumulatedMs_ = 0.0f;
    skipNextUpdate_ = false;
}

bool TextTyper::isComplete() const {
    return visibleChars_ >= codepoints_.size();
}

const std::string& TextTyper::text() const {
    return text_;
}

const std::vector<std::string>& TextTyper::codepoints() const {
    return codepoints_;
}

std::size_t TextTyper::visibleChars() const {
    return visibleChars_;
}

std::string TextTyper::visibleText() const {
    return utf8::join(codepoints_, 0, visibleChars_);
}
