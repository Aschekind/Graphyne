/**
 * @file window.h
 * @brief Window management for Graphyne
 */

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace graphyne::platform
{

/**
 * @class Window
 * @brief Class for creating and managing a window using SDL2
 */
class Window
{
public:
    /**
     * @brief Constructor
     * @param width Width of the window
     * @param height Height of the window
     * @param title Title of the window
     */
    Window(int width, int height, const std::string& title);

    /**
     * @brief Destructor
     */
    ~Window();

    /**
     * @brief Initialize the window
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the window
     */
    void shutdown();

    /**
     * @brief Process window events
     */
    void processEvents();

    /**
     * @brief Get the SDL window
     * @return Pointer to the SDL window
     */
    SDL_Window* getSDLWindow() const
    {
        return m_window;
    }

    /**
     * @brief Check if the window should close
     * @return True if the window should close, false otherwise
     */
    bool shouldClose() const
    {
        return m_shouldClose;
    }

    /**
     * @brief Get the width of the window
     * @return Width of the window in pixels
     */
    int getWidth() const
    {
        return m_width;
    }

    /**
     * @brief Get the height of the window
     * @return Height of the window in pixels
     */
    int getHeight() const
    {
        return m_height;
    }

    /**
     * @brief Get the required extensions for Vulkan
     * @return Vector of required extension names
     */
    std::vector<const char*> getRequiredExtensions() const;

private:
    SDL_Window* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    bool m_shouldClose;
};

} // namespace graphyne::platform
