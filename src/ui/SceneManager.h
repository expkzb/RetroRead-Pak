#pragma once

#include <memory>

class Renderer;
class Scene;

class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager();

    void setRoot(std::unique_ptr<Scene> scene);
    void replace(std::unique_ptr<Scene> scene);

    void update(float dt);
    void render(Renderer& renderer);
    bool shouldRenderContinuously() const;
    bool consumeRenderRequest();

private:
    std::unique_ptr<Scene> current_;
};
