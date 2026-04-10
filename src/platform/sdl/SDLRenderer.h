#pragma once

#include <string>
#include <unordered_map>

#include <SDL.h>
#ifndef NEXTREADING_NO_SDL_TTF
#include <SDL_ttf.h>
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include "platform/Renderer.h"

class SDLRenderer final : public Renderer {
public:
    SDLRenderer();
    ~SDLRenderer() override;

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
    bool saveScreenshot(const std::string& path) override;

    int screenWidth() const override;
    int screenHeight() const override;

private:
#ifndef NEXTREADING_NO_SDL_TTF
    TTF_Font* fontForSize(int fontSize, FontPreset fontPreset);
#else
    FT_Face faceForPreset(FontPreset fontPreset);
#endif
    std::string findFontPath(FontPreset fontPreset) const;
    SDL_Color toSdlColor(const Color& color) const;
    SDL_Rect toSdlRect(const Rect& rect) const;

    int width_ = 1280;
    int height_ = 720;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
#ifndef NEXTREADING_NO_SDL_TTF
    std::unordered_map<int, TTF_Font*> fontCache_;
#else
    FT_Library ftLibrary_ = nullptr;
    FT_Face normalFace_ = nullptr;
    FT_Face pixelFace_ = nullptr;
    std::string normalFacePath_;
    std::string pixelFacePath_;
#endif
};
