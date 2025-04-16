#include "graphyne/platform/window.h"
#include "graphyne/utils/logger.h"
#include <SDL2/SDL_vulkan.h>

namespace graphyne::platform
{

Window::Window(int width, int height, const std::string& title)
    : m_window(nullptr), m_width(width), m_height(height), m_title(title), m_shouldClose(false)
{
}

Window::~Window()
{
    shutdown();
}

bool Window::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        GN_ERROR("SDL initialization failed: {}", SDL_GetError());
        return false;
    }

    m_window = SDL_CreateWindow(m_title.c_str(),
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                m_width,
                                m_height,
                                SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!m_window)
    {
        GN_ERROR("Failed to create window: {}", SDL_GetError());
        return false;
    }

    GN_INFO("Window created successfully: {}x{}", m_width, m_height);
    return true;
}

void Window::shutdown()
{
    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

void Window::processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                m_shouldClose = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    m_width = event.window.data1;
                    m_height = event.window.data2;
                    GN_INFO("Window resized to {}x{}", m_width, m_height);
                }
                break;
            default:
                break;
        }
    }
}

std::vector<const char*> Window::getRequiredExtensions() const
{
    uint32_t count = 0;
    SDL_Vulkan_GetInstanceExtensions(m_window, &count, nullptr);

    std::vector<const char*> extensions(count);
    SDL_Vulkan_GetInstanceExtensions(m_window, &count, extensions.data());

    return extensions;
}

} // namespace graphyne::platform
