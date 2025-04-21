/**
 * @file vulkan_renderer.h
 * @brief Vulkan rendering backend implementation
 */
#pragma once

#include "graphyne/graphics/renderer.hpp"
#include "graphyne/core/memory.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace graphyne::graphics
{

/**
 * @class VulkanRenderer
 * @brief Vulkan implementation of the Renderer interface
 */
class VulkanRenderer : public Renderer
{
public:
    /**
     * @brief Constructor
     * @param window Window to render to
     * @param config Renderer configuration
     */
    VulkanRenderer(platform::Window& window, const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~VulkanRenderer() override;

    /**
     * @brief Initialize the Vulkan renderer
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize() override;

    /**
     * @brief Shutdown the Vulkan renderer
     */
    void shutdown() override;

    /**
     * @brief Begin a new frame
     */
    void beginFrame() override;

    /**
     * @brief End the current frame and present it
     */
    void endFrame() override;

    /**
     * @brief Wait for the device to be idle
     */
    void waitIdle() override;

    /**
     * @brief Handle window resize events
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void onResize(int width, int height) override;

private:
    // Vulkan instance and debugging
    bool createInstance();
    bool setupDebugMessenger();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

    // Device selection and creation
    bool pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool createLogicalDevice();

    // Swapchain
    bool createSurface();
    bool createSwapChain();
    void cleanupSwapChain();
    bool recreateSwapChain();

    // Vulkan resources
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    core::Vector<VkImage, core::AllocationType::Graphics> m_swapChainImages;
    core::Vector<VkImageView, core::AllocationType::Graphics> m_swapChainImageViews;
    VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_swapChainExtent = {0, 0};

    // Frame management
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;

    // Validation layers
    core::Vector<const char*, core::AllocationType::Graphics> m_validationLayers;

    // Device extensions
    core::Vector<const char*, core::AllocationType::Graphics> m_deviceExtensions;

    // Helper functions for validation layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
};

} // namespace graphyne::graphics
