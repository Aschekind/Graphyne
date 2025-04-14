#include "graphics/renderer.h"
#include "graphics/vulkan_renderer.h"
#include "utils/logger.h"

namespace graphyne::graphics
{

Renderer::Renderer(platform::Window& window, const Config& config) : m_window(window), m_config(config) {}

std::unique_ptr<Renderer> Renderer::create(platform::Window& window, const Config& config)
{
    // For now, we only support Vulkan
    auto renderer = std::make_unique<VulkanRenderer>(window, config);
    if (!renderer->initialize())
    {
        GN_ERROR("Failed to create Vulkan renderer");
        return nullptr;
    }
    return renderer;
}

} // namespace graphyne::graphics
