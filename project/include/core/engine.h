/**
 * @file engine.h
 * @brief Main engine class for Graphyne
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace graphyne {

// Forward declarations
namespace graphics {
class Renderer;
}

namespace platform {
class Window;
}

/**
 * @class Engine
 * @brief Main engine class that manages the game loop and subsystems
 */
class Engine
{
public:
    /**
     * @struct Config
     * @brief Configuration for initializing the engine
     */
    struct Config
    {
        std::string appName = "Graphyne Application";
        uint32_t windowWidth = 1280;
        uint32_t windowHeight = 720;
        bool enableValidation = true;
        bool enableVSync = true;
        bool m_shouldClose;
    };

    /**
     * @brief Constructor
     * @param config Engine configuration
     */
    explicit Engine(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~Engine();

    // Disable copy and move
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    /**
     * @brief Initialize the engine and all subsystems
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize();

    /**
     * @brief Shut down the engine and all subsystems
     */
    void shutdown();

    /**
     * @brief Run the main engine loop
     * @return Exit code (0 for success)
     */
    int run();

    /**
     * @brief Check if the engine is running
     * @return True if the engine is running, false otherwise
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Stop the engine
     */
    void stop() { m_running = false; }

private:
    Config m_config;
    bool m_initialized = false;
    bool m_running = false;

    // Core subsystems
    std::unique_ptr<platform::Window> m_window;
    std::unique_ptr<graphics::Renderer> m_renderer;

    // Engine loop methods
    void processEvents();
    void update(float deltaTime);
    void render();
};

} // namespace graphyne
