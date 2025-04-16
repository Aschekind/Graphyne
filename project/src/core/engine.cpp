#include "graphyne/core/engine.hpp"
#include "graphyne/graphics/renderer.hpp"
#include "graphyne/platform/window.hpp"
#include "graphyne/utils/logger.hpp"

namespace graphyne
{

Engine::Engine(const Config& config) : m_config(config) {}

Engine::~Engine()
{
    shutdown();
}

bool Engine::initialize()
{
    if (m_initialized)
    {
        GN_WARNING("Engine already initialized");
        return true;
    }

    // Initialize logger first
    if (!utils::Logger::getInstance().initialize())
    {
        GN_ERROR("Failed to initialize logger");
        return false;
    }

    GN_INFO("Initializing Graphyne Engine");

    // Create window
    m_window = std::make_unique<platform::Window>(m_config.windowWidth, m_config.windowHeight, m_config.appName);
    if (!m_window->initialize())
    {
        GN_ERROR("Failed to initialize window");
        return false;
    }

    // Create renderer
    graphics::Renderer::Config rendererConfig;
    rendererConfig.appName = m_config.appName;
    rendererConfig.enableValidation = m_config.enableValidation;
    rendererConfig.enableVSync = m_config.enableVSync;

    m_renderer = graphics::Renderer::create(*m_window, rendererConfig);
    if (!m_renderer || !m_renderer->initialize())
    {
        GN_ERROR("Failed to initialize renderer");
        return false;
    }

    m_initialized = true;
    GN_INFO("Engine initialized successfully");
    return true;
}

void Engine::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    GN_INFO("Shutting down Graphyne Engine");

    if (m_renderer)
    {
        m_renderer->shutdown();
        m_renderer.reset();
    }

    if (m_window)
    {
        m_window->shutdown();
        m_window.reset();
    }

    m_initialized = false;
    GN_INFO("Engine shutdown complete");
}

int Engine::run()
{
    if (!m_initialized)
    {
        GN_ERROR("Engine not initialized");
        return -1;
    }

    m_running = true;
    GN_INFO("Starting engine main loop");

    while (m_running)
    {
        processEvents();
        update(0.016f); // TODO: Implement proper delta time calculation
        render();
    }

    return 0;
}

void Engine::processEvents()
{
    m_window->processEvents();

    if (m_window->shouldClose())
    {
        m_running = false;
        GN_INFO("Window close requested, stopping engine loop");
    }
}

void Engine::update(float deltaTime)
{
    // TODO: Implement game logic update
}

void Engine::render()
{
    m_renderer->beginFrame();
    // TODO: Implement actual rendering
    m_renderer->endFrame();
}

} // namespace graphyne
