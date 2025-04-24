/**
 * @file renderer.h
 * @brief Graphics renderer interface
 */
#pragma once

#include <memory>
#include <string>

namespace graphyne::platform
{
class Window;
}

namespace graphyne::graphics
{

/**
 * @class Renderer
 * @brief Abstract base class for rendering backends
 */
class Renderer
{
public:
    /**
     * @struct Config
     * @brief Configuration options for the renderer
     */
    struct Config
    {
        std::string appName;
        uint32_t appVersion;
        bool enableValidation;
        bool enableVSync;

        Config() : appName("Graphyne Application"), appVersion(1), enableValidation(true), enableVSync(true) {}
    };

    /**
     * @brief Constructor
     * @param window Window to render to
     * @param config Renderer configuration
     */
    Renderer(platform::Window& window, const Config& config = Config());

    /**
     * @brief Virtual destructor
     */
    virtual ~Renderer() = default;

    // Disable copy and move
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /**
     * @brief Initialize the renderer
     * @return True if initialization succeeded, false otherwise
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown the renderer
     */
    virtual void shutdown() = 0;

    /**
     * @brief Begin a new frame
     */
    virtual void beginFrame() = 0;

    /**
     * @brief End the current frame and present it
     */
    virtual void endFrame() = 0;

    /**
     * @brief Wait for the device to be idle
     */
    virtual void waitIdle() = 0;

    /**
     * @brief Handle window resize events
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void onResize(int width, int height) = 0;

    /**
     * @brief Create a new renderer instance
     * @param window Window to render to
     * @param config Renderer configuration
     * @return Unique pointer to the created renderer
     */
    static std::unique_ptr<Renderer> create(platform::Window& window, const Config& config = Config());

protected:
    platform::Window& m_window;
    Config m_config;
};

} // namespace graphyne::graphics
