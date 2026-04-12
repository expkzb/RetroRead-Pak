#pragma once

#include "core/BookTypes.h"
#include "ui/AppScene.h"

class SettingsScene : public AppScene {
public:
    SettingsScene(Application& app, BookScript book, ReadingProgress progress);

    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    void applyDelta(int delta);
    void returnToReader();
    void clampScroll();

    BookScript book_;
    ReadingProgress progress_;
    int selectedIndex_ = 0;
    int scrollOffset_ = 0;
};
