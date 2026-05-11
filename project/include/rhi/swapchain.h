/**
 * @file rhi/swapchain.h
 * @brief Vulkan swapchain wrapper with recreate-on-resize support.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

class Swapchain {
public:
    Swapchain() = default;
    ~Swapchain();

    Swapchain(const Swapchain&)            = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&)                 = delete;
    Swapchain& operator=(Swapchain&&)      = delete;

    /// Create / recreate the swapchain. `desired_extent` is the window's
    /// drawable size in pixels.
    Result<void> initialize(Device& device, Extent2D desired_extent, bool vsync);

    /// Destroy the swapchain and its image views.
    void shutdown();

    /// Recreate after a window resize. Internally calls shutdown() + initialize().
    Result<void> recreate(Extent2D desired_extent, bool vsync);

    /// Acquire the next swapchain image. Returns:
    ///   true  + valid out_index on success
    ///   false + invalidated=true if the swapchain is out of date / suboptimal
    bool acquire_next_image(VkSemaphore signal, u32& out_index, bool& out_invalidated);

    /// Submit a present operation for the previously acquired image.
    /// Returns false if the swapchain became out-of-date.
    bool present(VkQueue queue, VkSemaphore wait, u32 image_index);

    VkSwapchainKHR        handle()       const { return m_swapchain; }
    VkFormat              image_format() const { return m_format; }
    Extent2D              extent()       const { return m_extent; }
    const std::vector<VkImage>&     images()      const { return m_images; }
    const std::vector<VkImageView>& image_views() const { return m_views;  }

private:
    Device*                  m_device     = nullptr;
    VkSwapchainKHR           m_swapchain  = VK_NULL_HANDLE;
    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_views;
    VkFormat                 m_format     = VK_FORMAT_UNDEFINED;
    Extent2D                 m_extent     = {0, 0};
    bool                     m_vsync      = true;
};

} // namespace gn::rhi
