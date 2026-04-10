#pragma once

#include <string>

#include "platform/Renderer.h"

struct NextUIRenderHooks {
    void* userdata = nullptr;
    bool (*initialize)(void* userdata, int* width, int* height) = nullptr;
    void (*shutdown)(void* userdata) = nullptr;
    void (*beginFrame)(void* userdata) = nullptr;
    void (*endFrame)(void* userdata) = nullptr;
    void (*clear)(void* userdata, Color color) = nullptr;
    void (*fillRect)(void* userdata, Rect rect, Color color) = nullptr;
    void (*drawRect)(void* userdata, Rect rect, Color color) = nullptr;
    void (*drawText)(
        void* userdata,
        const char* text,
        Rect bounds,
        Color color,
        int fontSize,
        TextAlign align,
        FontPreset fontPreset) = nullptr;
    int (*measureTextWidth)(
        void* userdata,
        const char* text,
        int fontSize,
        FontPreset fontPreset) = nullptr;
    int (*lineHeight)(void* userdata, int fontSize, FontPreset fontPreset) = nullptr;
};

class NextUIRenderer final : public Renderer {
public:
    static void registerHooks(const NextUIRenderHooks& hooks);

    bool initialize() override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;

    void clear(const Color& color) override;
    void fillRect(const Rect& rect, const Color& color) override;
    void drawRect(const Rect& rect, const Color& color) override;
    void drawText(
        const std::string& text,
        const Rect& bounds,
        const Color& color,
        int fontSize,
        TextAlign align,
        FontPreset fontPreset) override;

    int measureTextWidth(const std::string& text, int fontSize, FontPreset fontPreset) const override;
    int lineHeight(int fontSize, FontPreset fontPreset) const override;

    int screenWidth() const override;
    int screenHeight() const override;

private:
    int renderFontSize(int fontSize, FontPreset fontPreset) const;
    int fallbackMeasureTextWidth(const std::string& text, int fontSize, FontPreset fontPreset) const;
    int fallbackLineHeight(int fontSize, FontPreset fontPreset) const;

private:
    int width_ = 1024;
    int height_ = 768;
};
