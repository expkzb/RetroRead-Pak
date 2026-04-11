#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class DialogueStyle : std::uint8_t {
    Classic = 0,
    Bold = 1,
    Calm = 2,
    Frame = 3,
    Battle = 4,
};

enum class FontPreset : std::uint8_t {
    Normal = 0,
    Pixel = 1,
};

enum class TextVoiceMode : std::uint8_t {
    Off = 0,
    Fixed = 1,
    FollowText = 2,
};

enum class PerformanceMode : std::uint8_t {
    Off = 0,
    Hud = 1,
    Log = 2,
};

struct Sentence {
    std::string text;
    std::uint32_t chapterIndex = 0;
    std::uint32_t paragraphIndex = 0;
    std::uint32_t sentenceIndex = 0;
    bool isTitle = false;
};

struct Chapter {
    std::string id;
    std::string title;
    std::vector<Sentence> sentences;
};

struct BookScript {
    std::string bookId;
    std::string title;
    std::string author;
    std::string sourcePath;
    std::vector<Chapter> chapters;
};

struct ReadingProgress {
    std::string bookId;
    std::uint32_t chapterIndex = 0;
    std::uint32_t sentenceIndex = 0;
    bool autoPlay = false;
    std::uint32_t textSpeed = 30;
    std::uint64_t lastOpenedAt = 0;
};

struct ReaderSettings {
    std::uint32_t textSpeed = 30;
    bool defaultAutoPlay = false;
    TextVoiceMode textVoiceMode = TextVoiceMode::Off;
    std::uint32_t fontSize = 30;
    DialogueStyle dialogueStyle = DialogueStyle::Classic;
    FontPreset fontPreset = FontPreset::Normal;
    PerformanceMode performanceMode = PerformanceMode::Off;
};

struct BookListItem {
    std::string path;
    std::string title;
};
