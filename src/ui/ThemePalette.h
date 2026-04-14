#pragma once

#include "core/BookTypes.h"
#include "platform/Renderer.h"

struct ThemePalette {
    Color screenBackground;
    Color headerText;
    Color primaryText;
    Color secondaryText;
    Color accentText;
    Color accentSecondaryText;
    Color selectionFill;
    Color selectionOutline;
    Color selectionText;
    Color selectionSubtext;
    Color dialoguePanel;
    Color dialogueBorder;
    Color dialogueInnerBorder;
    Color dialogueTitle;
    Color dialogueBody;
    Color dialogueHint;
    int dialogueBorderThickness = 1;
    bool dialogueInnerFrame = false;
    bool dialogueBattleFrame = false;
};

inline ThemePalette themePalette(ThemePreset preset) {
    switch (preset) {
    case ThemePreset::BoldAmber:
        return ThemePalette{
            Color{12, 18, 28, 255},
            Color{255, 233, 188, 255},
            Color{240, 243, 248, 255},
            Color{174, 194, 214, 255},
            Color{255, 233, 188, 255},
            Color{227, 211, 172, 255},
            Color{88, 66, 26, 255},
            Color{255, 210, 120, 255},
            Color{255, 245, 220, 255},
            Color{255, 223, 166, 255},
            Color{12, 18, 28, 236},
            Color{255, 210, 120, 255},
            Color{96, 132, 164, 255},
            Color{255, 233, 188, 255},
            Color{240, 243, 248, 255},
            Color{174, 194, 214, 255},
            3,
            false,
            false};
    case ThemePreset::CalmTeal:
        return ThemePalette{
            Color{10, 28, 30, 255},
            Color{214, 245, 235, 255},
            Color{230, 240, 238, 255},
            Color{160, 210, 208, 255},
            Color{214, 245, 235, 255},
            Color{160, 210, 208, 255},
            Color{34, 78, 80, 255},
            Color{124, 214, 204, 255},
            Color{230, 248, 244, 255},
            Color{186, 228, 224, 255},
            Color{10, 28, 30, 224},
            Color{112, 176, 170, 255},
            Color{56, 82, 110, 255},
            Color{214, 245, 235, 255},
            Color{230, 240, 238, 255},
            Color{160, 210, 208, 255},
            1,
            false,
            false};
    case ThemePreset::FrameBlue:
        return ThemePalette{
            Color{14, 19, 30, 255},
            Color{246, 230, 179, 255},
            Color{232, 238, 246, 255},
            Color{174, 194, 214, 255},
            Color{246, 230, 179, 255},
            Color{214, 200, 164, 255},
            Color{46, 70, 110, 255},
            Color{148, 176, 212, 255},
            Color{244, 248, 252, 255},
            Color{210, 222, 236, 255},
            Color{14, 19, 30, 238},
            Color{148, 176, 212, 255},
            Color{67, 98, 138, 255},
            Color{246, 230, 179, 255},
            Color{240, 243, 248, 255},
            Color{174, 194, 214, 255},
            1,
            true,
            false};
    case ThemePreset::BattleRed:
        return ThemePalette{
            Color{20, 12, 18, 255},
            Color{255, 225, 160, 255},
            Color{245, 236, 232, 255},
            Color{255, 186, 156, 255},
            Color{255, 225, 160, 255},
            Color{255, 186, 156, 255},
            Color{110, 42, 30, 255},
            Color{226, 113, 86, 255},
            Color{255, 241, 230, 255},
            Color{255, 205, 176, 255},
            Color{20, 12, 18, 238},
            Color{226, 113, 86, 255},
            Color{255, 204, 120, 255},
            Color{255, 225, 160, 255},
            Color{245, 236, 232, 255},
            Color{255, 186, 156, 255},
            2,
            true,
            true};
    case ThemePreset::MintLcd:
        return ThemePalette{
            Color{188, 202, 160, 255},
            Color{34, 48, 30, 255},
            Color{30, 42, 26, 255},
            Color{74, 92, 60, 255},
            Color{20, 30, 18, 255},
            Color{50, 68, 42, 255},
            Color{82, 104, 62, 255},
            Color{44, 60, 34, 255},
            Color{230, 240, 214, 255},
            Color{206, 222, 182, 255},
            Color{202, 214, 176, 245},
            Color{70, 90, 54, 255},
            Color{126, 146, 94, 255},
            Color{24, 34, 20, 255},
            Color{30, 42, 26, 255},
            Color{74, 92, 60, 255},
            2,
            true,
            false};
    case ThemePreset::GbInvert:
        return ThemePalette{
            Color{8, 24, 32, 255},
            Color{224, 248, 208, 255},
            Color{224, 248, 208, 255},
            Color{136, 192, 112, 255},
            Color{224, 248, 208, 255},
            Color{136, 192, 112, 255},
            Color{224, 248, 208, 255},
            Color{136, 192, 112, 255},
            Color{8, 24, 32, 255},
            Color{52, 104, 86, 255},
            Color{52, 104, 86, 244},
            Color{136, 192, 112, 255},
            Color{52, 104, 86, 255},
            Color{224, 248, 208, 255},
            Color{224, 248, 208, 255},
            Color{136, 192, 112, 255},
            2,
            true,
            false};
    case ThemePreset::ClassicDark:
    default:
        return ThemePalette{
            Color{10, 14, 20, 255},
            Color{255, 233, 188, 255},
            Color{240, 243, 248, 255},
            Color{174, 194, 214, 255},
            Color{255, 233, 188, 255},
            Color{227, 211, 172, 255},
            Color{34, 46, 64, 255},
            Color{96, 132, 164, 255},
            Color{244, 248, 252, 255},
            Color{205, 221, 237, 255},
            Color{16, 22, 32, 224},
            Color{96, 132, 164, 255},
            Color{56, 82, 110, 255},
            Color{255, 233, 188, 255},
            Color{240, 243, 248, 255},
            Color{174, 194, 214, 255},
            1,
            false,
            false};
    }
}
