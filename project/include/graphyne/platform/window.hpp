#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace graphyne::platform
{

class Window
{
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool initialize();
    void shutdown();
    void processEvents();

    SDL_Window* getSDLWindow() const
    {
        return m_window;
    }
    bool shouldClose() const
    {
        return m_shouldClose;
    }
    int getWidth() const
    {
        return m_width;
    }
    int getHeight() const
    {
        return m_height;
    }

    std::vector<const char*> getRequiredExtensions() const;

private:
    SDL_Window* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    bool m_shouldClose;
};

} // namespace graphyne::platform
