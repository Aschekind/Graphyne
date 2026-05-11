/**
 * @file core/engine.h
 * @brief Engine — owns subsystems and runs the main loop.
 *
 * Typical usage:
 *
 *   gn::Engine::Config cfg;
 *   cfg.app_name      = "My Game";
 *   cfg.window_width  = 1280;
 *   cfg.window_height = 720;
 *
 *   gn::Engine engine(cfg);
 *   MyGame game;
 *   return engine.run(game);
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <memory>
#include <string>

namespace gn {

class App;

namespace platform { class Platform; class Window; class Input; }
namespace rhi      { class Device; class Swapchain; }
namespace graphics { class Renderer; }

class Engine {
public:
    struct Config {
        std::string app_name        = "Graphyne Application";
        u32         window_width    = 1280;
        u32         window_height   = 720;
        bool        resizable       = true;
        bool        vsync           = true;
        bool        enable_validation = true;
        // Path to a log file. Empty -> console only.
        std::string log_file;
    };

    explicit Engine(Config config = {});
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(Engine&&)      = delete;

    /// Initialize all subsystems. Safe to call exactly once.
    Result<void> initialize();

    /// Tear down all subsystems. Safe to call multiple times.
    void shutdown();

    /// Run the main loop driving the supplied App. Returns the application's
    /// exit code (0 on a graceful exit).
    int run(App& app);

    /// Ask the loop to terminate after the current frame.
    void request_exit() { m_running = false; }

    bool initialized() const { return m_initialized; }
    bool running()     const { return m_running; }

    // Accessors for App / subsystem code that needs them.
    platform::Window&  window();
    platform::Input&   input();
    graphics::Renderer& renderer();

    const Config& config() const { return m_config; }

private:
    Config m_config;
    bool   m_initialized = false;
    bool   m_running     = false;

    std::unique_ptr<platform::Platform>  m_platform;
    std::unique_ptr<platform::Window>    m_window;
    std::unique_ptr<platform::Input>     m_input;
    std::unique_ptr<rhi::Device>         m_device;
    std::unique_ptr<rhi::Swapchain>      m_swapchain;
    std::unique_ptr<graphics::Renderer>  m_renderer;
};

} // namespace gn
