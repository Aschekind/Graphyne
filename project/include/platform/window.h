/**
 * @file platform/window.h
 * @brief OS window backed by SDL2.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"
#include "platform/event.h"

#include <functional>
#include <string>
#include <vector>

struct SDL_Window;

namespace gn::platform {

class Window {
public:
    struct Config {
        std::string title  = "Graphyne";
        u32  width         = 1280;
        u32  height        = 720;
        bool resizable     = true;
        bool high_dpi      = true;
    };

    using EventHandler = std::function<void(const Event&)>;

    Window() = default;
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&)                 = delete;
    Window& operator=(Window&&)      = delete;

    /// Create the underlying SDL window. Requires the Platform to be initialized.
    Result<void> initialize(const Config& cfg);

    /// Destroy the underlying SDL window. Safe to call repeatedly.
    void shutdown();

    /// Pump platform events into `handler`. Updates internal close/resize state.
    void pump_events(const EventHandler& handler);

    bool should_close() const { return m_should_close; }
    void set_should_close(bool v) { m_should_close = v; }

    u32  width()   const { return m_extent.width; }
    u32  height()  const { return m_extent.height; }
    Extent2D extent() const { return m_extent; }
    bool minimized() const { return m_minimized; }

    /// Vulkan instance extensions required by the windowing system.
    std::vector<const char*> required_instance_extensions() const;

    /// Native handle for surface creation. Owned by Window.
    SDL_Window* native() const { return m_window; }

private:
    SDL_Window* m_window       = nullptr;
    Extent2D    m_extent       = {0, 0};
    bool        m_should_close = false;
    bool        m_minimized    = false;
};

} // namespace gn::platform
