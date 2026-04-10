#include "platform/PlatformFactory.h"

#include <memory>

#include "platform/Clock.h"
#include "platform/FileSystem.h"
#include "platform/Input.h"
#include "platform/Renderer.h"
#include "platform/nextui/NextUIClock.h"
#include "platform/nextui/NextUIFileSystem.h"
#include "platform/nextui/NextUIInput.h"
#include "platform/nextui/NextUIRenderer.h"
#include "platform/sdl/SDLClock.h"
#include "platform/sdl/SDLFileSystem.h"
#include "platform/sdl/SDLInput.h"
#include "platform/sdl/SDLRenderer.h"

PlatformServices PlatformFactory::create(PlatformBackend backend) {
    switch (backend) {
    case PlatformBackend::NextUI:
        return PlatformServices{
            std::make_unique<NextUIRenderer>(),
            std::make_unique<NextUIInput>(),
            std::make_unique<NextUIFileSystem>(),
            std::make_unique<NextUIClock>(),
        };
    case PlatformBackend::SDL:
    default:
        return PlatformServices{
            std::make_unique<SDLRenderer>(),
            std::make_unique<SDLInput>(),
            std::make_unique<SDLFileSystem>(),
            std::make_unique<SDLClock>(),
        };
    }
}
