#pragma once

#include <string>

#include "core/BookTypes.h"
#include "ui/AppScene.h"

class ChapterScene : public AppScene {
public:
    ChapterScene(Application& app, BookScript book);

    void onEnter() override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    BookScript book_;
    ReadingProgress savedProgress_;
    bool hasSavedProgress_ = false;
    int selectedIndex_ = 0;
};
