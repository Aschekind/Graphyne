#include "platform/platform.h"

#include "utils/logger.h"

#include <SDL2/SDL.h>

namespace gn::platform {

Platform::~Platform() {
    shutdown();
}

Result<void> Platform::initialize() {
    if (m_initialized) return Ok();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return Err{Error{std::string("SDL_Init failed: ") + SDL_GetError()}};
    }
    m_initialized = true;
    GN_INFO("Platform initialized (SDL {}.{}.{})",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    return Ok();
}

void Platform::shutdown() {
    if (!m_initialized) return;
    SDL_Quit();
    m_initialized = false;
}

} // namespace gn::platform
