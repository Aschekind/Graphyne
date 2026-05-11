#include "core/engine.h"

#include "core/app.h"
#include "core/clock.h"
#include "graphics/renderer.h"
#include "platform/input.h"
#include "platform/platform.h"
#include "platform/window.h"
#include "rhi/device.h"
#include "rhi/swapchain.h"
#include "utils/logger.h"

namespace gn {

Engine::Engine(Config config) : m_config(std::move(config)) {}

Engine::~Engine() { shutdown(); }

Result<void> Engine::initialize() {
    if (m_initialized) return Ok();

    utils::Logger::instance().initialize(m_config.log_file, utils::LogLevel::Debug);
    GN_INFO("Initializing engine");

    m_platform = std::make_unique<platform::Platform>();
    if (auto r = m_platform->initialize(); !r) {
        GN_ERROR("Platform init failed: {}", r.error().what());
        return Err{r.error()};
    }

    m_window = std::make_unique<platform::Window>();
    platform::Window::Config wcfg;
    wcfg.title     = m_config.app_name;
    wcfg.width     = m_config.window_width;
    wcfg.height    = m_config.window_height;
    wcfg.resizable = m_config.resizable;
    if (auto r = m_window->initialize(wcfg); !r) {
        GN_ERROR("Window init failed: {}", r.error().what());
        return Err{r.error()};
    }

    m_input = std::make_unique<platform::Input>();

    m_device = std::make_unique<rhi::Device>();
    rhi::Device::Config dcfg;
    dcfg.app_name          = m_config.app_name;
    dcfg.enable_validation = m_config.enable_validation;
    if (auto r = m_device->initialize(*m_window, dcfg); !r) {
        GN_ERROR("Device init failed: {}", r.error().what());
        return Err{r.error()};
    }

    m_swapchain = std::make_unique<rhi::Swapchain>();
    if (auto r = m_swapchain->initialize(*m_device, m_window->extent(), m_config.vsync); !r) {
        GN_ERROR("Swapchain init failed: {}", r.error().what());
        return Err{r.error()};
    }

    m_renderer = std::make_unique<graphics::Renderer>();
    graphics::Renderer::Config rcfg;
    rcfg.vsync = m_config.vsync;
    if (auto r = m_renderer->initialize(*m_device, *m_swapchain, *m_window, rcfg); !r) {
        GN_ERROR("Renderer init failed: {}", r.error().what());
        return Err{r.error()};
    }

    m_initialized = true;
    GN_INFO("Engine initialized");
    return Ok();
}

void Engine::shutdown() {
    if (!m_initialized) return;
    GN_INFO("Shutting down engine");
    if (m_device) m_device->wait_idle();

    m_renderer.reset();
    m_swapchain.reset();
    m_device.reset();
    m_input.reset();
    m_window.reset();
    m_platform.reset();

    utils::Logger::instance().shutdown();
    m_initialized = false;
}

int Engine::run(App& app) {
    if (!m_initialized) {
        if (auto r = initialize(); !r) {
            return 1;
        }
    }

    app.m_engine = this;
    if (!app.on_start()) {
        GN_ERROR("App::on_start returned false; aborting");
        return 1;
    }

    Clock clock;
    m_running = true;

    while (m_running && !m_window->should_close()) {
        const f32 dt = clock.tick();

        m_input->begin_frame();
        bool resized = false;
        u32  new_w   = 0;
        u32  new_h   = 0;
        m_window->pump_events([&](const platform::Event& e) {
            m_input->on_event(e);
            app.on_event(e);
            if (e.type == platform::EventType::WindowResize) {
                resized = true;
                new_w   = e.resize.width;
                new_h   = e.resize.height;
            }
        });

        if (resized) {
            m_renderer->on_resize();
            app.on_resize(new_w, new_h);
        }

        app.on_update(dt);

        if (graphics::Frame* frame = m_renderer->begin_frame(clock.elapsed(), dt)) {
            app.on_render(*frame);
            m_renderer->end_frame();
        }
    }

    if (m_device) m_device->wait_idle();
    app.on_shutdown();
    app.m_engine = nullptr;
    return 0;
}

platform::Window&  Engine::window()   { return *m_window; }
platform::Input&   Engine::input()    { return *m_input; }
graphics::Renderer& Engine::renderer() { return *m_renderer; }

} // namespace gn
