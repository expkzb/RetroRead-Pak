#include "ui/SceneManager.h"

#include "ui/Scene.h"

SceneManager::~SceneManager() = default;

void SceneManager::setRoot(std::unique_ptr<Scene> scene) {
    current_ = std::move(scene);
    if (current_) {
        current_->onEnter();
        renderRequested_ = true;
    }
}

void SceneManager::replace(std::unique_ptr<Scene> scene) {
    if (current_) {
        current_->onExit();
    }

    current_ = std::move(scene);
    if (current_) {
        current_->onEnter();
        renderRequested_ = true;
    }
}

void SceneManager::update(float dt) {
    if (current_) {
        current_->update(dt);
    }
}

void SceneManager::render(Renderer& renderer) {
    if (current_) {
        current_->render(renderer);
    }
}

bool SceneManager::shouldRenderContinuously() const {
    return current_ != nullptr && current_->shouldRenderContinuously();
}

bool SceneManager::consumeRenderRequest() {
    bool requested = renderRequested_;
    renderRequested_ = false;
    if (current_ != nullptr && current_->consumeRenderRequest()) {
        requested = true;
    }
    return requested;
}
