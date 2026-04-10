#include "audio/TextBlipPlayer.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cctype>

#include "text/Utf8.h"

namespace {
constexpr float kFollowCooldownMs = 18.0f;
constexpr float kFixedBeatMs = 72.0f;

bool isAsciiWordChar(const std::string& codepoint) {
    if (codepoint.size() != 1) {
        return false;
    }
    const unsigned char ch = static_cast<unsigned char>(codepoint[0]);
    return std::isalnum(ch) != 0 || ch == '\'' || ch == '_';
}

std::uint32_t codepointSeed(const std::string& codepoint) {
    std::uint32_t seed = 2166136261u;
    for (unsigned char ch : codepoint) {
        seed ^= static_cast<std::uint32_t>(ch);
        seed *= 16777619u;
    }
    return seed;
}
}

bool TextBlipPlayer::initialize() {
    if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            return false;
        }
    }

    SDL_AudioSpec desired{};
    desired.freq = sampleRate_;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 512;

    SDL_AudioSpec obtained{};
    deviceId_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (deviceId_ == 0) {
        return false;
    }

    sampleRate_ = obtained.freq;
    SDL_PauseAudioDevice(deviceId_, 0);
    return true;
}

void TextBlipPlayer::shutdown() {
    if (deviceId_ != 0) {
        SDL_ClearQueuedAudio(deviceId_);
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }
}

void TextBlipPlayer::update(float dtSeconds) {
    timerMs_ = std::max(0.0f, timerMs_ - dtSeconds * 1000.0f);
}

void TextBlipPlayer::reset() {
    timerMs_ = 0.0f;
    sequence_ = 0;
    spokenVisibleCount_ = 0;
    codepoints_.clear();
    if (deviceId_ != 0) {
        SDL_ClearQueuedAudio(deviceId_);
    }
}

void TextBlipPlayer::playCodepoint(const std::string& codepoint, const std::string& previousCodepoint) {
    if (timerMs_ > 0.0f || !isSpeakable(codepoint, previousCodepoint)) {
        timerMs_ = std::max(timerMs_, pauseAfter(codepoint));
        return;
    }

    if (emitBlip(codepoint)) {
        timerMs_ += kFollowCooldownMs + pauseAfter(codepoint);
    }
}

void TextBlipPlayer::syncVisibleCodepoints(const std::vector<std::string>& codepoints, std::size_t visibleCount) {
    codepoints_ = codepoints;
    const std::size_t cappedVisible = std::min(visibleCount, codepoints_.size());
    if (spokenVisibleCount_ > cappedVisible) {
        spokenVisibleCount_ = cappedVisible;
    }

    while (spokenVisibleCount_ < cappedVisible && timerMs_ <= 0.0f) {
        const std::string& codepoint = codepoints_[spokenVisibleCount_];
        const std::string previous = spokenVisibleCount_ > 0 ? codepoints_[spokenVisibleCount_ - 1] : std::string{};
        ++spokenVisibleCount_;

        if (isFixedBeatSpeakable(codepoint)) {
            emitBlip(codepoint);
            timerMs_ += kFixedBeatMs + pauseAfter(codepoint);
        } else {
            timerMs_ += pauseAfter(codepoint);
        }
    }
}

bool TextBlipPlayer::isSpeakable(const std::string& codepoint, const std::string& previousCodepoint) const {
    if (codepoint.empty() || utf8::isWhitespace(codepoint)) {
        return false;
    }

    if (isAsciiWordChar(codepoint) && isAsciiWordChar(previousCodepoint)) {
        return false;
    }

    static const char* kAsciiSkips = ".,!?;:-_+=/\\|()[]{}\"'`~";
    if (codepoint.size() == 1 && !isAsciiWordChar(codepoint)) {
        for (const char* p = kAsciiSkips; *p != '\0'; ++p) {
            if (codepoint[0] == *p) {
                return false;
            }
        }
    }

    return codepoint != u8"，" && codepoint != u8"。" && codepoint != u8"！" && codepoint != u8"？" &&
           codepoint != u8"、" && codepoint != u8"：" && codepoint != u8"；" && codepoint != u8"（" &&
           codepoint != u8"）" && codepoint != u8"《" && codepoint != u8"》";
}

bool TextBlipPlayer::isFixedBeatSpeakable(const std::string& codepoint) const {
    if (codepoint.empty() || utf8::isWhitespace(codepoint)) {
        return false;
    }

    static const char* kAsciiSkips = ".,!?;:-_+=/\\|()[]{}\"'`~";
    if (codepoint.size() == 1) {
        for (const char* p = kAsciiSkips; *p != '\0'; ++p) {
            if (codepoint[0] == *p) {
                return false;
            }
        }
    }

    return codepoint != u8"，" && codepoint != u8"。" && codepoint != u8"！" && codepoint != u8"？" &&
           codepoint != u8"、" && codepoint != u8"：" && codepoint != u8"；" && codepoint != u8"（" &&
           codepoint != u8"）" && codepoint != u8"《" && codepoint != u8"》";
}

float TextBlipPlayer::pauseAfter(const std::string& codepoint) const {
    if (utf8::isWhitespace(codepoint)) {
        return 18.0f;
    }
    if (codepoint == "," || codepoint == ";" || codepoint == ":" || codepoint == u8"，" ||
        codepoint == u8"、" || codepoint == u8"；" || codepoint == u8"：") {
        return 110.0f;
    }
    if (codepoint == "." || codepoint == "!" || codepoint == "?" || codepoint == u8"。" ||
        codepoint == u8"！" || codepoint == u8"？") {
        return 190.0f;
    }
    return 0.0f;
}

bool TextBlipPlayer::emitBlip(const std::string& codepoint) {
    if (deviceId_ == 0) {
        return false;
    }

    const std::uint32_t seed = codepointSeed(codepoint) + sequence_++;
    const std::vector<std::int16_t> wave = buildWave(seed);
    if (wave.empty()) {
        return false;
    }

    SDL_QueueAudio(deviceId_, wave.data(), static_cast<Uint32>(wave.size() * sizeof(std::int16_t)));
    return true;
}

std::vector<std::int16_t> TextBlipPlayer::buildWave(std::uint32_t seed) const {
    const int durationMs = 20 + static_cast<int>(seed % 6);
    const int sampleCount = std::max(1, sampleRate_ * durationMs / 1000);
    const float baseFreq = 680.0f + static_cast<float>(seed % 7) * 36.0f;
    const float duty = 0.32f + static_cast<float>((seed >> 3) % 5) * 0.03f;
    const float gain = 0.12f;

    std::vector<std::int16_t> samples(static_cast<std::size_t>(sampleCount));
    for (int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate_);
        const float progress = static_cast<float>(i) / static_cast<float>(sampleCount);
        const float env = (1.0f - progress) * (1.0f - progress);
        const float phase = std::fmod(t * baseFreq, 1.0f);
        const float square = phase < duty ? 1.0f : -1.0f;
        const float tri = 2.0f * std::fabs(2.0f * phase - 1.0f) - 1.0f;
        const float sample = (square * 0.72f + tri * 0.28f) * env * gain;
        samples[static_cast<std::size_t>(i)] =
            static_cast<std::int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
    }

    return samples;
}
