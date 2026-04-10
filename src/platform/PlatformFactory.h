#pragma once

#include <memory>

class Clock;
class FileSystem;
class Input;
class Renderer;

enum class PlatformBackend {
    SDL,
    NextUI,
};

struct PlatformServices {
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Input> input;
    std::unique_ptr<FileSystem> fileSystem;
    std::unique_ptr<Clock> clock;
};

class PlatformFactory {
public:
    static PlatformServices create(PlatformBackend backend);
};
