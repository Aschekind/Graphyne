#include "graphics/vulkan_renderer.h"
#include "platform/window.h"
#include "utils/logger.h"
#include <set>
#include <stdexcept>

namespace graphyne::graphics
{

VulkanRenderer::VulkanRenderer(platform::Window& window, const Config& config) : Renderer(window, config) {}

VulkanRenderer::~VulkanRenderer()
{
    shutdown();
}

bool VulkanRenderer::initialize()
{
    if (!createInstance())
    {
        GN_ERROR("Failed to create Vulkan instance");
        return false;
    }

    if (m_config.enableValidation && !setupDebugMessenger())
    {
        GN_ERROR("Failed to setup debug messenger");
        return false;
    }

    if (!createSurface())
    {
        GN_ERROR("Failed to create surface");
        return false;
    }

    if (!pickPhysicalDevice())
    {
        GN_ERROR("Failed to pick physical device");
        return false;
    }

    if (!createLogicalDevice())
    {
        GN_ERROR("Failed to create logical device");
        return false;
    }

    if (!createSwapChain())
    {
        GN_ERROR("Failed to create swap chain");
        return false;
    }

    GN_INFO("Vulkan renderer initialized successfully");
    return true;
}

void VulkanRenderer::shutdown()
{
    cleanupSwapChain();

    if (m_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_debugMessenger != VK_NULL_HANDLE)
    {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::beginFrame()
{
    // TODO: Implement frame begin
}

void VulkanRenderer::endFrame()
{
    // TODO: Implement frame end
}

void VulkanRenderer::waitIdle()
{
    vkDeviceWaitIdle(m_device);
}

void VulkanRenderer::onResize(int width, int height)
{
    m_framebufferResized = true;
}

bool VulkanRenderer::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_config.appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Graphyne";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_config.enableValidation)
    {
        if (!checkValidationLayerSupport())
        {
            GN_ERROR("Validation layers requested but not available");
            return false;
        }

        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;

        createInfo.pNext = &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        GN_ERROR("Failed to create Vulkan instance");
        return false;
    }

    return true;
}

bool VulkanRenderer::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == nullptr)
    {
        GN_ERROR("Failed to load vkCreateDebugUtilsMessengerEXT");
        return false;
    }

    if (func(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
        GN_ERROR("Failed to setup debug messenger");
        return false;
    }

    return true;
}

bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions()
{
    std::vector<const char*> extensions;

    // Get required extensions from window
    auto windowExtensions = m_window.getRequiredExtensions();
    extensions.insert(extensions.end(), windowExtensions.begin(), windowExtensions.end());

    if (m_config.enableValidation)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanRenderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        GN_ERROR("Failed to find GPUs with Vulkan support");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        GN_ERROR("Failed to find a suitable GPU");
        return false;
    }

    return true;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device)
{
    // TODO: Implement proper device suitability check
    return true;
}

bool VulkanRenderer::createLogicalDevice()
{
    // TODO: Implement logical device creation
    return true;
}

bool VulkanRenderer::createSurface()
{
    // TODO: Implement surface creation
    return true;
}

bool VulkanRenderer::createSwapChain()
{
    // TODO: Implement swap chain creation
    return true;
}

void VulkanRenderer::cleanupSwapChain()
{
    // TODO: Implement swap chain cleanup
}

bool VulkanRenderer::recreateSwapChain()
{
    // TODO: Implement swap chain recreation
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void* pUserData)
{
    std::string message = "Validation layer: " + std::string(pCallbackData->pMessage);

    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            GN_DEBUG("Validation layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            GN_INFO("Validation layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            GN_WARNING("Validation layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            GN_ERROR("Validation layer: {}", pCallbackData->pMessage);
            break;
        default:
            break;
    }
    return VK_FALSE;
}

} // namespace graphyne::graphics
