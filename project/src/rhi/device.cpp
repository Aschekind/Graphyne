#include "rhi/device.h"

#include "platform/window.h"
#include "utils/logger.h"

#include <SDL2/SDL_vulkan.h>

#include <cstring>
#include <set>
#include <string>

namespace gn::rhi {

namespace {

constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*)
{
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            GN_TRACE("[vk] {}", data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            GN_DEBUG("[vk] {}", data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            GN_WARNING("[vk] {}", data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            GN_ERROR("[vk] {}", data->pMessage); break;
        default: break;
    }
    return VK_FALSE;
}

bool check_validation_support() {
    u32 count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& l : layers) {
        if (std::strcmp(l.layerName, kValidationLayer) == 0) return true;
    }
    return false;
}

VkDebugUtilsMessengerCreateInfoEXT make_debug_create_info() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debug_callback;
    return info;
}

} // namespace

Device::~Device() { shutdown(); }

Result<void> Device::initialize(platform::Window& window, const Config& cfg) {
    if (m_device != VK_NULL_HANDLE) return Ok();

    m_validation = cfg.enable_validation && check_validation_support();
    if (cfg.enable_validation && !m_validation) {
        GN_WARNING("Validation layers requested but not available — continuing without them.");
    }

    auto window_exts = window.required_instance_extensions();
    auto r1 = create_instance(cfg, window_exts);
    if (!r1) return r1;

    if (m_validation) {
        auto r2 = setup_debug_messenger();
        if (!r2) return r2;
    }

    // Surface (SDL handles the OS-specific bits).
    if (!SDL_Vulkan_CreateSurface(window.native(), m_instance, &m_surface)) {
        return Err{Error{std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError()}};
    }

    auto r3 = pick_physical_device();
    if (!r3) return r3;
    auto r4 = create_logical_device();
    if (!r4) return r4;

    GN_INFO("Vulkan device ready (graphics={}, present={})",
            m_queue_families.graphics, m_queue_families.present);
    return Ok();
}

void Device::shutdown() {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    if (m_debug_messenger != VK_NULL_HANDLE) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (fn) fn(m_instance, m_debug_messenger, nullptr);
        m_debug_messenger = VK_NULL_HANDLE;
    }
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
    m_physical_device = VK_NULL_HANDLE;
    m_graphics_queue  = VK_NULL_HANDLE;
    m_present_queue   = VK_NULL_HANDLE;
}

void Device::wait_idle() const {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
}

Result<void> Device::create_instance(const Config& cfg, const std::vector<const char*>& window_exts) {
    VkApplicationInfo app{};
    app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName   = cfg.app_name.c_str();
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName        = "Graphyne";
    app.engineVersion      = VK_MAKE_VERSION(0, 2, 0);
    app.apiVersion         = cfg.api_version;

    std::vector<const char*> exts(window_exts.begin(), window_exts.end());
    if (m_validation) {
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo info{};
    info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo        = &app;
    info.enabledExtensionCount   = static_cast<u32>(exts.size());
    info.ppEnabledExtensionNames = exts.data();

    VkDebugUtilsMessengerCreateInfoEXT dbg = make_debug_create_info();
    if (m_validation) {
        info.enabledLayerCount   = 1;
        info.ppEnabledLayerNames = &kValidationLayer;
        info.pNext               = &dbg;
    }

    VkResult res = vkCreateInstance(&info, nullptr, &m_instance);
    if (res != VK_SUCCESS) {
        return Err{Error{"vkCreateInstance failed (code " + std::to_string(res) + ")"}};
    }
    return Ok();
}

Result<void> Device::setup_debug_messenger() {
    auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!fn) {
        return Err{Error{"vkCreateDebugUtilsMessengerEXT not found"}};
    }
    auto info = make_debug_create_info();
    VkResult res = fn(m_instance, &info, nullptr, &m_debug_messenger);
    if (res != VK_SUCCESS) {
        return Err{Error{"vkCreateDebugUtilsMessengerEXT failed"}};
    }
    return Ok();
}

QueueFamilyIndices Device::find_queue_families(VkPhysicalDevice device) const {
    QueueFamilyIndices idx;
    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (u32 i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphics = i;

        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present);
        if (present) idx.present = i;

        if (idx.complete()) break;
    }
    return idx;
}

bool Device::check_device_extensions(VkPhysicalDevice device) const {
    u32 count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

    std::set<std::string> required(m_device_extensions.begin(), m_device_extensions.end());
    for (const auto& ext : available) required.erase(ext.extensionName);
    return required.empty();
}

bool Device::is_device_suitable(VkPhysicalDevice device) const {
    auto qf = find_queue_families(device);
    if (!qf.complete()) return false;
    if (!check_device_extensions(device)) return false;

    // Confirm at least one surface format / present mode exists.
    u32 fmt_count = 0, pm_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &fmt_count, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &pm_count, nullptr);
    return fmt_count > 0 && pm_count > 0;
}

Result<void> Device::pick_physical_device() {
    u32 count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) return Err{Error{"No Vulkan-capable GPUs found"}};

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    // Prefer discrete GPUs.
    VkPhysicalDevice fallback = VK_NULL_HANDLE;
    for (auto d : devices) {
        if (!is_device_suitable(d)) continue;
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(d, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_physical_device = d;
            break;
        }
        if (fallback == VK_NULL_HANDLE) fallback = d;
    }
    if (m_physical_device == VK_NULL_HANDLE) m_physical_device = fallback;
    if (m_physical_device == VK_NULL_HANDLE) {
        return Err{Error{"No suitable GPU found (missing required extensions or queues)"}};
    }

    m_queue_families = find_queue_families(m_physical_device);
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(m_physical_device, &props);
    GN_INFO("Picked GPU: {} (Vulkan {}.{}.{})",
            props.deviceName,
            VK_VERSION_MAJOR(props.apiVersion),
            VK_VERSION_MINOR(props.apiVersion),
            VK_VERSION_PATCH(props.apiVersion));
    return Ok();
}

Result<void> Device::create_logical_device() {
    std::set<u32> unique_families = {m_queue_families.graphics, m_queue_families.present};
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    queue_infos.reserve(unique_families.size());

    const float prio = 1.0f;
    for (u32 family : unique_families) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount       = 1;
        qi.pQueuePriorities = &prio;
        queue_infos.push_back(qi);
    }

    // Vulkan 1.3 features: dynamic rendering + synchronization2.
    VkPhysicalDeviceVulkan13Features f13{};
    f13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f13.dynamicRendering = VK_TRUE;
    f13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &f13;

    VkDeviceCreateInfo dci{};
    dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pNext                   = &features2;
    dci.queueCreateInfoCount    = static_cast<u32>(queue_infos.size());
    dci.pQueueCreateInfos       = queue_infos.data();
    dci.enabledExtensionCount   = static_cast<u32>(m_device_extensions.size());
    dci.ppEnabledExtensionNames = m_device_extensions.data();
    // pEnabledFeatures must be null when using features2 via pNext.

    VkResult res = vkCreateDevice(m_physical_device, &dci, nullptr, &m_device);
    if (res != VK_SUCCESS) {
        return Err{Error{"vkCreateDevice failed (code " + std::to_string(res) + ")"}};
    }

    vkGetDeviceQueue(m_device, m_queue_families.graphics, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_device, m_queue_families.present,  0, &m_present_queue);
    return Ok();
}

} // namespace gn::rhi
