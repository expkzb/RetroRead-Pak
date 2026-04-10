#pragma once

#include "ui/Scene.h"

class Application;

class AppScene : public Scene {
public:
    explicit AppScene(Application& app) : app_(app) {}
    virtual ~AppScene() = default;

protected:
    Application& app_;
};
