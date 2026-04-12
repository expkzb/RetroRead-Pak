#include <algorithm>

#include "ui/widgets/DialogueBox.h"
#include "ui/ThemePalette.h"

namespace {
int uiSpacing(int normalValue, int handheldValue) {
#ifdef NEXTREADING_TG5040
    (void)normalValue;
    return handheldValue;
#else
    (void)handheldValue;
    return normalValue;
#endif
}

int dialogueBodyFont(const ReaderSettings& settings) {
#ifdef NEXTREADING_TG5040
    return std::max(24, std::min(48, static_cast<int>(settings.fontSize) + 8));
#else
    return std::max(20, std::min(36, static_cast<int>(settings.fontSize)));
#endif
}

int dialogueTitleFont() {
#ifdef NEXTREADING_TG5040
    return 30;
#else
    return 20;
#endif
}

int dialogueHintFont() {
#ifdef NEXTREADING_TG5040
    return 24;
#else
    return 16;
#endif
}

void drawInsetFrame(Renderer& renderer, const Rect& bounds, const Color& outer, const Color& inner) {
    renderer.drawRect(bounds, outer);
    renderer.drawRect(Rect{bounds.x + 2, bounds.y + 2, bounds.w - 4, bounds.h - 4}, inner);
}

void fillRectSafe(Renderer& renderer, const Rect& rect, const Color& color) {
    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }
    renderer.fillRect(rect, color);
}

void drawCutCornerFrame(Renderer& renderer, const Rect& bounds, int thickness, int cutSize, const Color& color) {
    if (bounds.w <= cutSize * 2 || bounds.h <= cutSize * 2 || thickness <= 0) {
        return;
    }

    fillRectSafe(renderer, Rect{bounds.x + cutSize, bounds.y, bounds.w - cutSize * 2, thickness}, color);
    fillRectSafe(renderer, Rect{bounds.x + cutSize, bounds.y + bounds.h - thickness, bounds.w - cutSize * 2, thickness}, color);
    fillRectSafe(renderer, Rect{bounds.x, bounds.y + cutSize, thickness, bounds.h - cutSize * 2}, color);
    fillRectSafe(renderer, Rect{bounds.x + bounds.w - thickness, bounds.y + cutSize, thickness, bounds.h - cutSize * 2}, color);

    fillRectSafe(renderer, Rect{bounds.x, bounds.y + cutSize, cutSize, thickness}, color);
    fillRectSafe(renderer, Rect{bounds.x + cutSize, bounds.y, thickness, cutSize}, color);

    fillRectSafe(renderer, Rect{bounds.x + bounds.w - cutSize, bounds.y, thickness, cutSize}, color);
    fillRectSafe(renderer, Rect{bounds.x + bounds.w - cutSize, bounds.y + cutSize, cutSize, thickness}, color);

    fillRectSafe(renderer, Rect{bounds.x, bounds.y + bounds.h - cutSize - thickness, cutSize, thickness}, color);
    fillRectSafe(renderer, Rect{bounds.x + cutSize, bounds.y + bounds.h - cutSize, thickness, cutSize}, color);

    fillRectSafe(renderer, Rect{bounds.x + bounds.w - cutSize, bounds.y + bounds.h - cutSize, thickness, cutSize}, color);
    fillRectSafe(
        renderer,
        Rect{bounds.x + bounds.w - cutSize, bounds.y + bounds.h - cutSize - thickness, cutSize, thickness},
        color);
}
}

void DialogueBox::setBounds(const Rect& rect) {
    bounds_ = rect;
}

void DialogueBox::setTitle(const std::string& title) {
    title_ = title;
}

void DialogueBox::setBodyLines(const std::vector<std::string>& lines) {
    bodyLines_ = lines;
}

void DialogueBox::setBodyRevealTexts(const std::vector<std::string>& revealTexts) {
    bodyRevealTexts_ = revealTexts;
}

void DialogueBox::setHint(const std::string& hint) {
    hint_ = hint;
}

void DialogueBox::render(Renderer& renderer, const ReaderSettings& settings) {
    const ThemePalette palette = themePalette(settings.themePreset);
    const Color panel = palette.dialoguePanel;
    const Color border = palette.dialogueBorder;
    const Color titleColor = palette.dialogueTitle;
    const Color bodyColor = palette.dialogueBody;
    const Color hintColor = palette.dialogueHint;
    const int borderThickness = palette.dialogueBorderThickness;
    const bool drawInnerFrame = palette.dialogueInnerFrame;
    const bool useBattleFrame = palette.dialogueBattleFrame;
    const Color innerBorder = palette.dialogueInnerBorder;

    renderer.fillRect(bounds_, panel);
    if (useBattleFrame) {
        const int cut = uiSpacing(8, 12);
        drawCutCornerFrame(renderer, bounds_, borderThickness, cut, border);
    } else {
        for (int i = 0; i < borderThickness; ++i) {
            renderer.drawRect(Rect{bounds_.x - i, bounds_.y - i, bounds_.w + i * 2, bounds_.h + i * 2}, border);
        }
    }
    if (drawInnerFrame && bounds_.w > 12 && bounds_.h > 12) {
        const Rect innerRect{bounds_.x + 4, bounds_.y + 4, bounds_.w - 8, bounds_.h - 8};
        if (useBattleFrame) {
            drawCutCornerFrame(renderer, innerRect, 2, uiSpacing(6, 10), innerBorder);
        } else {
            drawInsetFrame(renderer, innerRect, innerBorder, border);
        }
    }

    const int titleFont = dialogueTitleFont();
    const int bodyFont = dialogueBodyFont(settings);
    const int hintFont = dialogueHintFont();
    const int titleHeight = renderer.lineHeight(titleFont, settings.fontPreset);
    const int bodyHeight = renderer.lineHeight(bodyFont, settings.fontPreset);
    const int hintHeight = renderer.lineHeight(hintFont, settings.fontPreset);
    const int innerX = bounds_.x + 24;
    const int innerWidth = bounds_.w - 48;
    const int bodyRenderWidth = innerWidth + uiSpacing(8, 14);
    const int titleY = bounds_.y + 16;
    const int bodyY = titleY + titleHeight + uiSpacing(14, 22);
    const int lineStep = bodyHeight + uiSpacing(6, 12);
    const int hintY = bounds_.y + bounds_.h - hintHeight - uiSpacing(10, 14);
    renderer.drawText(title_, Rect{innerX, titleY, innerWidth, titleHeight + 6}, titleColor, titleFont, TextAlign::Left, settings.fontPreset);

    int y = bodyY;
    for (std::size_t i = 0; i < bodyLines_.size(); ++i) {
        if (i < bodyRevealTexts_.size() && !bodyRevealTexts_[i].empty()) {
            renderer.drawText(
                bodyRevealTexts_[i],
                Rect{innerX, y, bodyRenderWidth, bodyHeight + 8},
                bodyColor,
                bodyFont,
                TextAlign::Left,
                settings.fontPreset);
        }
        y += lineStep;
    }

    renderer.drawText(
        hint_,
        Rect{innerX, hintY, innerWidth, hintHeight + 4},
        hintColor,
        hintFont,
        TextAlign::Right,
        settings.fontPreset);
}
