#include "platform/nextui/NextUIBindings.h"

#include "platform/nextui/NextUIInput.h"
#include "platform/nextui/NextUIRenderer.h"

namespace {
struct NextUIDeviceContext {
};

bool initializeRenderer(void* userdata, int* width, int* height) {
    (void)userdata;

    if (width != nullptr && *width <= 0) {
        *width = 1024;
    }
    if (height != nullptr && *height <= 0) {
        *height = 768;
    }

    // TODO(trimui-brick): initialize the real NextUI/MinUI drawing backend here.
    return true;
}

void shutdownRenderer(void* userdata) {
    (void)userdata;
    // TODO(trimui-brick): shut down device drawing resources here.
}

void beginFrame(void* userdata) {
    (void)userdata;
    // TODO(trimui-brick): begin a device frame here if the platform requires it.
}

void endFrame(void* userdata) {
    (void)userdata;
    // TODO(trimui-brick): present the frame here if the platform requires it.
}

void clear(void* userdata, Color color) {
    (void)userdata;
    (void)color;
    // TODO(trimui-brick): clear the framebuffer.
}

void fillRect(void* userdata, Rect rect, Color color) {
    (void)userdata;
    (void)rect;
    (void)color;
    // TODO(trimui-brick): fill a solid rectangle.
}

void drawRect(void* userdata, Rect rect, Color color) {
    (void)userdata;
    (void)rect;
    (void)color;
    // TODO(trimui-brick): draw a rectangle border.
}

void drawText(
    void* userdata,
    const char* text,
    Rect bounds,
    Color color,
    int fontSize,
    TextAlign align,
    FontPreset fontPreset) {
    (void)userdata;
    (void)text;
    (void)bounds;
    (void)color;
    (void)fontSize;
    (void)align;
    (void)fontPreset;
    // TODO(trimui-brick): draw UTF-8 text with the device font API.
}

int measureTextWidth(void* userdata, const char* text, int fontSize, FontPreset fontPreset) {
    (void)userdata;
    (void)text;
    (void)fontSize;
    (void)fontPreset;
    // Returning -1 keeps NextUIRenderer on its fallback width estimator until real text metrics are wired.
    return -1;
}

int lineHeight(void* userdata, int fontSize, FontPreset fontPreset) {
    (void)userdata;
    (void)fontSize;
    (void)fontPreset;
    // Returning -1 keeps NextUIRenderer on its fallback line-height estimator until real font metrics are wired.
    return -1;
}

bool initializeInput(void* userdata) {
    (void)userdata;
    // TODO(trimui-brick): initialize the real button/input subsystem here.
    return true;
}

void shutdownInput(void* userdata) {
    (void)userdata;
    // TODO(trimui-brick): release button/input resources here.
}

void pollButtons(void* userdata, bool* buttons, std::size_t buttonCount, bool* quitRequested) {
    (void)userdata;
    (void)quitRequested;

    for (std::size_t i = 0; i < buttonCount; ++i) {
        buttons[i] = false;
    }

    // TODO(trimui-brick): fill buttons[] with the current device button state.
    // Expected order matches NextUIButton:
    // A, B, X, Y, L1, R1, Start, Select, Up, Down, Left, Right
}
}  // namespace

void installNextUIBindings() {
    static NextUIDeviceContext context;

    NextUIRenderer::registerHooks(NextUIRenderHooks{
        &context,
        &initializeRenderer,
        &shutdownRenderer,
        &beginFrame,
        &endFrame,
        &clear,
        &fillRect,
        &drawRect,
        &drawText,
        &measureTextWidth,
        &lineHeight,
    });

    NextUIInput::registerHooks(NextUIInputHooks{
        &context,
        &initializeInput,
        &shutdownInput,
        &pollButtons,
    });
}
