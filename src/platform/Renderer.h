#pragma once

#include <cstdint>
#include <string>

#include "core/BookTypes.h"

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

enum class TextAlign {
    Left,
    Center,
    Right,
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void clear(const Color& color) = 0;
    virtual void fillRect(const Rect& rect, const Color& color) = 0;
    virtual void drawRect(const Rect& rect, const Color& color) = 0;
    virtual void drawText(
        const std::string& text,
        const Rect& bounds,
        const Color& color,
        int fontSize,
        TextAlign align = TextAlign::Left,
        FontPreset fontPreset = FontPreset::Normal) = 0;

    virtual int measureTextWidth(const std::string& text, int fontSize, FontPreset fontPreset = FontPreset::Normal) const = 0;
    virtual int lineHeight(int fontSize, FontPreset fontPreset = FontPreset::Normal) const = 0;

    virtual int screenWidth() const = 0;
    virtual int screenHeight() const = 0;
};
