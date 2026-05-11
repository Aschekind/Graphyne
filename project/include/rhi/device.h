/**
 * @file rhi/device.h
 * @brief Vulkan device and queue management.
 *
 * Wraps VkInstance, VkPhysicalDevice, VkDevice, the graphics+present
 * queues, the surface, and the optional debug messenger. All other RHI
 * objects (swapchain, command pools, sync) reference back into this one.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace gn::platform { class Window; }

namespace gn::rhi {

struct QueueFamilyIndices {
    u32  graphics = ~0u;
    u32  present  = ~0u;
    bool complete() const { return graphics != ~0u && present != ~0u; }
};

class Device {
public:
    struct Config {
        std::string app_name        = "Graphyne Application";
        bool        enable_validation = true;
        // Vulkan API version target. 1.3 enables dynamic rendering / sync2.
        u32         api_version     = VK_API_VERSION_1_3;
    };

    Device() = default;
    ~Device();

    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&)                 = delete;
    Device& operator=(Device&&)      = delete;

    /// Initialize the Vulkan instance, surface, physical device, and logical device.
    Result<void> initialize(platform::Window& window, const Config& cfg = {});

    /// Tear down all Vulkan objects. Safe to call multiple times.
    void shutdown();

    /// Block until all GPU work on this device has completed.
    void wait_idle() const;

    VkInstance        instance()         const { return m_instance; }
    VkPhysicalDevice  physical_device()  const { return m_physical_device; }
    VkDevice          logical_device()   const { return m_device; }
    VkSurfaceKHR      surface()          const { return m_surface; }
    VkQueue           graphics_queue()   const { return m_graphics_queue; }
    VkQueue           present_queue()    const { return m_present_queue; }
    QueueFamilyIndices queue_families()  const { return m_queue_families; }
    bool              validation_enabled() const { return m_validation; }
    u32               effective_api_version() const { return m_effective_api_version; }

    /// Find a memory type index satisfying `type_filter` (from
    /// VkMemoryRequirements::memoryTypeBits) AND containing all of
    /// `required_flags`. Returns ~0u on failure.
    u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags required_flags) const;

    /// Run a one-shot command on the graphics queue (allocates a transient
    /// command buffer, executes the lambda, submits, and waits for completion).
    /// Used by upload paths (texture/buffer staging).
    void submit_one_shot(const std::function<void(VkCommandBuffer)>& fn);

private:
    Result<void> create_instance(const Config& cfg, const std::vector<const char*>& window_exts);
    Result<void> setup_debug_messenger();
    Result<void> pick_physical_device();
    Result<void> create_logical_device();
    bool         is_device_suitable(VkPhysicalDevice device) const;
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;
    bool         check_device_extensions(VkPhysicalDevice device) const;

    VkInstance               m_instance        = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface         = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physical_device = VK_NULL_HANDLE;
    VkDevice                 m_device          = VK_NULL_HANDLE;
    VkQueue                  m_graphics_queue  = VK_NULL_HANDLE;
    VkQueue                  m_present_queue   = VK_NULL_HANDLE;
    QueueFamilyIndices       m_queue_families       = {};
    bool                     m_validation           = false;
    u32                      m_effective_api_version = 0;
    // One-shot command pool used by upload helpers.
    VkCommandPool            m_one_shot_pool        = VK_NULL_HANDLE;

    // Required device extensions (swapchain + anything we need for VK 1.3 dyn rendering).
    std::vector<const char*> m_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
};

} // namespace gn::rhi
