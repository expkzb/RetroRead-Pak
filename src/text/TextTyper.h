#pragma once

#include <cstddef>
#include <string>
#include <vector>

class TextTyper {
public:
    void start(const std::string& text, int baseSpeedMs);
    int update(float dtSeconds);

    void revealAll();
    bool isComplete() const;

    const std::string& text() const;
    const std::vector<std::string>& codepoints() const;
    std::size_t visibleChars() const;
    std::string visibleText() const;

private:
    std::string text_;
    std::vector<std::string> codepoints_;
    std::size_t visibleChars_ = 0;
    float accumulatedMs_ = 0.0f;
    int baseSpeedMs_ = 30;
    bool skipNextUpdate_ = false;
};
