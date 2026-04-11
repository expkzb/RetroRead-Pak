#pragma once

#include <string>

#include "ui/AppScene.h"

enum class LoadingTarget {
    Reader,
    Chapters,
};

class LoadingScene : public AppScene {
public:
    LoadingScene(Application& app, std::string bookPath, std::string bookTitle, LoadingTarget target);

    void onEnter() override;
    void update(float dt) override;
    void render(Renderer& renderer) override;
    bool shouldRenderContinuously() const override { return true; }

private:
    void finishLoading();

    std::string bookPath_;
    std::string bookTitle_;
    LoadingTarget target_;
    bool started_ = false;
    bool failed_ = false;
    std::string errorMessage_;
};
