/**
 * @file platform/platform.h
 * @brief Global platform initialization (SDL).
 *
 * Owns the SDL_Init / SDL_Quit lifetime so that Window and Input can be
 * constructed and destroyed multiple times without tearing SDL down.
 */
#pragma once

#include "core/result.h"

namespace gn::platform {

class Platform {
public:
    Platform() = default;
    ~Platform();

    Platform(const Platform&)            = delete;
    Platform& operator=(const Platform&) = delete;
    Platform(Platform&&)                 = delete;
    Platform& operator=(Platform&&)      = delete;

    /// Initialize SDL subsystems (Video + Events). Idempotent.
    Result<void> initialize();

    /// Tear down SDL. Safe if never initialized.
    void shutdown();

    bool initialized() const { return m_initialized; }

private:
    bool m_initialized = false;
};

} // namespace gn::platform
