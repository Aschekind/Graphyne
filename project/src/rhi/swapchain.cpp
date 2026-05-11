#include "rhi/swapchain.h"

#include "rhi/device.h"
#include "utils/logger.h"

#include <algorithm>
#include <limits>

namespace gn::rhi {

namespace {

VkSurfaceFormatKHR choose_format(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return formats[0];
}

VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes, bool vsync) {
    if (vsync) return VK_PRESENT_MODE_FIFO_KHR;  // always available
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    }
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) return m;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& caps, Extent2D desired) {
    if (caps.currentExtent.width != std::numeric_limits<u32>::max()) {
        return caps.currentExtent;
    }
    VkExtent2D actual{desired.width, desired.height};
    actual.width  = std::clamp(actual.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    actual.height = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return actual;
}

} // namespace

Swapchain::~Swapchain() { shutdown(); }

Result<void> Swapchain::initialize(Device& device, Extent2D desired_extent, bool vsync) {
    m_device = &device;
    m_vsync  = vsync;

    const auto phys    = device.physical_device();
    const auto surface = device.surface();

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    u32 fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmt_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fmt_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmt_count, formats.data());

    u32 pm_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &pm_count, nullptr);
    std::vector<VkPresentModeKHR> modes(pm_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &pm_count, modes.data());

    const VkSurfaceFormatKHR fmt   = choose_format(formats);
    const VkPresentModeKHR   mode  = choose_present_mode(modes, vsync);
    const VkExtent2D         ext   = choose_extent(caps, desired_extent);

    u32 image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR sci{};
    sci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface          = surface;
    sci.minImageCount    = image_count;
    sci.imageFormat      = fmt.format;
    sci.imageColorSpace  = fmt.colorSpace;
    sci.imageExtent      = ext;
    sci.imageArrayLayers = 1;
    sci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    const auto qfi = device.queue_families();
    const u32 indices[] = {qfi.graphics, qfi.present};
    if (qfi.graphics != qfi.present) {
        sci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices   = indices;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    sci.preTransform   = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode    = mode;
    sci.clipped        = VK_TRUE;
    sci.oldSwapchain   = VK_NULL_HANDLE;

    VkDevice dev = device.logical_device();
    VkResult res = vkCreateSwapchainKHR(dev, &sci, nullptr, &m_swapchain);
    if (res != VK_SUCCESS) {
        return Err{Error{"vkCreateSwapchainKHR failed (" + std::to_string(res) + ")"}};
    }

    u32 actual_count = 0;
    vkGetSwapchainImagesKHR(dev, m_swapchain, &actual_count, nullptr);
    m_images.resize(actual_count);
    vkGetSwapchainImagesKHR(dev, m_swapchain, &actual_count, m_images.data());

    m_views.resize(actual_count);
    for (u32 i = 0; i < actual_count; ++i) {
        VkImageViewCreateInfo vci{};
        vci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image    = m_images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format   = fmt.format;
        vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel   = 0;
        vci.subresourceRange.levelCount     = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount     = 1;
        if (vkCreateImageView(dev, &vci, nullptr, &m_views[i]) != VK_SUCCESS) {
            return Err{Error{"vkCreateImageView failed"}};
        }
    }

    m_format = fmt.format;
    m_extent = Extent2D{ext.width, ext.height};
    GN_INFO("Swapchain created: {}x{} ({} images)", m_extent.width, m_extent.height, actual_count);
    return Ok();
}

void Swapchain::shutdown() {
    if (!m_device) return;
    VkDevice dev = m_device->logical_device();
    if (dev == VK_NULL_HANDLE) {
        m_swapchain = VK_NULL_HANDLE;
        m_views.clear();
        m_images.clear();
        return;
    }
    for (auto v : m_views) {
        if (v != VK_NULL_HANDLE) vkDestroyImageView(dev, v, nullptr);
    }
    m_views.clear();
    m_images.clear();
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(dev, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

Result<void> Swapchain::recreate(Extent2D desired_extent, bool vsync) {
    if (!m_device) return Err{Error{"Swapchain not initialized"}};
    Device* d = m_device;
    d->wait_idle();
    shutdown();
    return initialize(*d, desired_extent, vsync);
}

bool Swapchain::acquire_next_image(VkSemaphore signal, u32& out_index, bool& out_invalidated) {
    out_invalidated = false;
    if (!m_device || m_swapchain == VK_NULL_HANDLE) return false;

    VkResult res = vkAcquireNextImageKHR(m_device->logical_device(),
                                         m_swapchain,
                                         std::numeric_limits<u64>::max(),
                                         signal,
                                         VK_NULL_HANDLE,
                                         &out_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        out_invalidated = true;
        return false;
    }
    if (res == VK_SUBOPTIMAL_KHR) {
        out_invalidated = true;
        return true;
    }
    return res == VK_SUCCESS;
}

bool Swapchain::present(VkQueue queue, VkSemaphore wait, u32 image_index) {
    if (m_swapchain == VK_NULL_HANDLE) return false;

    VkPresentInfoKHR info{};
    info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = wait != VK_NULL_HANDLE ? 1u : 0u;
    info.pWaitSemaphores    = wait != VK_NULL_HANDLE ? &wait : nullptr;
    info.swapchainCount     = 1;
    info.pSwapchains        = &m_swapchain;
    info.pImageIndices      = &image_index;

    VkResult res = vkQueuePresentKHR(queue, &info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        return false;
    }
    return res == VK_SUCCESS;
}

} // namespace gn::rhi
