#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class TextBlipPlayer {
public:
    bool initialize();
    void shutdown();

    void update(float dtSeconds);
    void reset();
    void playCodepoint(const std::string& codepoint, const std::string& previousCodepoint = {});
    void syncVisibleCodepoints(const std::vector<std::string>& codepoints, std::size_t visibleCount);

private:
    bool isSpeakable(const std::string& codepoint, const std::string& previousCodepoint) const;
    bool isFixedBeatSpeakable(const std::string& codepoint) const;
    float pauseAfter(const std::string& codepoint) const;
    bool emitBlip(const std::string& codepoint);
    std::vector<std::int16_t> buildWave(std::uint32_t seed) const;

private:
    std::uint32_t deviceId_ = 0;
    int sampleRate_ = 22050;
    float timerMs_ = 0.0f;
    std::uint32_t sequence_ = 0;
    std::size_t spokenVisibleCount_ = 0;
    std::vector<std::string> codepoints_;
};
