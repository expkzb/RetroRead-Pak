#include "platform/nextui/NextUIRenderer.h"

#include <algorithm>
#include <cstdlib>

#include "text/Utf8.h"

namespace {
NextUIRenderHooks g_hooks;
}

void NextUIRenderer::registerHooks(const NextUIRenderHooks& hooks) {
    g_hooks = hooks;
}

bool NextUIRenderer::initialize() {
    if (g_hooks.initialize != nullptr) {
        int hookWidth = width_;
        int hookHeight = height_;
        if (!g_hooks.initialize(g_hooks.userdata, &hookWidth, &hookHeight)) {
            return false;
        }
        width_ = std::max(320, hookWidth);
        height_ = std::max(240, hookHeight);
        return true;
    }

    if (const char* width = std::getenv("NEXTUI_SCREEN_WIDTH"); width != nullptr && *width != '\0') {
        width_ = std::max(320, std::atoi(width));
    }

    if (const char* height = std::getenv("NEXTUI_SCREEN_HEIGHT"); height != nullptr && *height != '\0') {
        height_ = std::max(240, std::atoi(height));
    }

    return true;
}

void NextUIRenderer::shutdown() {
    if (g_hooks.shutdown != nullptr) {
        g_hooks.shutdown(g_hooks.userdata);
    }
}

void NextUIRenderer::beginFrame() {
    if (g_hooks.beginFrame != nullptr) {
        g_hooks.beginFrame(g_hooks.userdata);
    }
}

void NextUIRenderer::endFrame() {
    if (g_hooks.endFrame != nullptr) {
        g_hooks.endFrame(g_hooks.userdata);
    }
}

void NextUIRenderer::clear(const Color& color) {
    if (g_hooks.clear != nullptr) {
        g_hooks.clear(g_hooks.userdata, color);
    }
}

void NextUIRenderer::fillRect(const Rect& rect, const Color& color) {
    if (g_hooks.fillRect != nullptr) {
        g_hooks.fillRect(g_hooks.userdata, rect, color);
    }
}

void NextUIRenderer::drawRect(const Rect& rect, const Color& color) {
    if (g_hooks.drawRect != nullptr) {
        g_hooks.drawRect(g_hooks.userdata, rect, color);
    }
}

void NextUIRenderer::drawText(
    const std::string& text,
    const Rect& bounds,
    const Color& color,
    int fontSize,
    TextAlign align,
    FontPreset fontPreset) {
    if (text.empty()) {
        return;
    }

    if (g_hooks.drawText != nullptr) {
        g_hooks.drawText(g_hooks.userdata, text.c_str(), bounds, color, fontSize, align, fontPreset);
    }
}

int NextUIRenderer::measureTextWidth(const std::string& text, int fontSize, FontPreset fontPreset) const {
    if (g_hooks.measureTextWidth != nullptr) {
        return std::max(0, g_hooks.measureTextWidth(g_hooks.userdata, text.c_str(), fontSize, fontPreset));
    }

    return fallbackMeasureTextWidth(text, fontSize, fontPreset);
}

int NextUIRenderer::lineHeight(int fontSize, FontPreset fontPreset) const {
    if (g_hooks.lineHeight != nullptr) {
        return std::max(1, g_hooks.lineHeight(g_hooks.userdata, fontSize, fontPreset));
    }

    return fallbackLineHeight(fontSize, fontPreset);
}

bool NextUIRenderer::saveScreenshot(const std::string& path) {
    (void)path;
    return false;
}

int NextUIRenderer::screenWidth() const {
    return width_;
}

int NextUIRenderer::screenHeight() const {
    return height_;
}

int NextUIRenderer::renderFontSize(int fontSize, FontPreset fontPreset) const {
    if (fontPreset == FontPreset::Pixel) {
        return std::max(10, fontSize - 2);
    }

    return fontSize;
}

int NextUIRenderer::fallbackMeasureTextWidth(
    const std::string& text,
    int fontSize,
    FontPreset fontPreset) const {
    const auto codepoints = utf8::splitCodepoints(text);
    const int renderSize = renderFontSize(fontSize, fontPreset);
    int width = 0;

    for (const std::string& codepoint : codepoints) {
        if (utf8::isWhitespace(codepoint)) {
            width += std::max(3, renderSize / 3);
            continue;
        }

        const unsigned char lead = static_cast<unsigned char>(codepoint.front());
        const bool looksAscii = codepoint.size() == 1 && lead < 0x80;
        width += looksAscii ? std::max(4, (renderSize * 11) / 20) : std::max(6, renderSize);
    }

    return width;
}

int NextUIRenderer::fallbackLineHeight(int fontSize, FontPreset fontPreset) const {
    const int renderSize = renderFontSize(fontSize, fontPreset);
    return renderSize + std::max(6, renderSize / 3);
}
