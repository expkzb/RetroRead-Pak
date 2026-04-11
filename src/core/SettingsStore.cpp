#include "core/SettingsStore.h"

#include <sstream>
#include <string>

#include "platform/FileSystem.h"

namespace {
std::string settingsPath(FileSystem& fileSystem) {
    return fileSystem.savesPath() + "/settings.db";
}
}

bool SettingsStore::load(FileSystem& fileSystem, ReaderSettings& settings) const {
    std::string content;
    if (!fileSystem.readTextFile(settingsPath(fileSystem), content)) {
        return true;
    }

    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        std::stringstream lineStream(line);
        std::string key;
        std::string value;
        std::getline(lineStream, key, '\t');
        std::getline(lineStream, value, '\t');

        if (key == "textSpeed") {
            settings.textSpeed = static_cast<std::uint32_t>(std::stoul(value));
        } else if (key == "defaultAutoPlay") {
            settings.defaultAutoPlay = value == "1";
        } else if (key == "textBlipEnabled") {
            settings.textVoiceMode = value == "1" ? TextVoiceMode::FollowText : TextVoiceMode::Off;
        } else if (key == "textVoiceMode") {
            settings.textVoiceMode = static_cast<TextVoiceMode>(std::stoi(value));
        } else if (key == "fontSize") {
            settings.fontSize = static_cast<std::uint32_t>(std::stoul(value));
        } else if (key == "dialogueStyle") {
            settings.dialogueStyle = static_cast<DialogueStyle>(std::stoi(value));
        } else if (key == "fontPreset") {
            settings.fontPreset = static_cast<FontPreset>(std::stoi(value));
        } else if (key == "showPerformanceHud") {
            settings.performanceMode = value == "1" ? PerformanceMode::Hud : PerformanceMode::Off;
        } else if (key == "performanceMode") {
            settings.performanceMode = static_cast<PerformanceMode>(std::stoi(value));
        }
    }

    return true;
}

bool SettingsStore::save(FileSystem& fileSystem, const ReaderSettings& settings) const {
    fileSystem.createDirectories(fileSystem.savesPath());

    std::ostringstream out;
    out << "textSpeed\t" << settings.textSpeed << '\n';
    out << "defaultAutoPlay\t" << (settings.defaultAutoPlay ? 1 : 0) << '\n';
    out << "textVoiceMode\t" << static_cast<int>(settings.textVoiceMode) << '\n';
    out << "fontSize\t" << settings.fontSize << '\n';
    out << "dialogueStyle\t" << static_cast<int>(settings.dialogueStyle) << '\n';
    out << "fontPreset\t" << static_cast<int>(settings.fontPreset) << '\n';
    out << "performanceMode\t" << static_cast<int>(settings.performanceMode) << '\n';

    return fileSystem.writeTextFile(settingsPath(fileSystem), out.str());
}
