/**
 * @file rhi/image.h
 * @brief GPU image helpers — VkImage + VkImageView + VkDeviceMemory bundle.
 *
 * Manual allocation per image (no VMA yet). For textures, follow this flow:
 *   1. Image::initialize(device, w, h, format, usage)
 *   2. Image::upload_pixels(span_of_pixels) — handles staging + layout
 *      transitions to SHADER_READ_ONLY_OPTIMAL.
 *
 * The image is created with VK_IMAGE_LAYOUT_UNDEFINED and transitioned to
 * SHADER_READ_ONLY_OPTIMAL on upload_pixels().
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <span>
#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

struct ImageDesc {
    u32              width   = 0;
    u32              height  = 0;
    VkFormat         format  = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageUsageFlags usage  = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
};

class Image {
public:
    Image() = default;
    ~Image();

    Image(const Image&)            = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    Result<void> initialize(Device& device, const ImageDesc& desc);

    void shutdown();

    /// Upload tightly-packed pixel data of size `width * height * bytes_per_pixel`.
    /// On success the image layout is SHADER_READ_ONLY_OPTIMAL.
    Result<void> upload_pixels(std::span<const u8> pixels, VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImage        handle()        const { return m_image; }
    VkImageView    view()          const { return m_view; }
    VkDeviceMemory memory()        const { return m_memory; }
    VkFormat       format()        const { return m_desc.format; }
    u32            width()         const { return m_desc.width; }
    u32            height()        const { return m_desc.height; }
    VkImageLayout  current_layout() const { return m_layout; }

private:
    Device*        m_device = nullptr;
    VkImage        m_image  = VK_NULL_HANDLE;
    VkImageView    m_view   = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    ImageDesc      m_desc   = {};
    VkImageLayout  m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

/// Create a generic linear sampler. Caller owns the returned handle.
VkSampler create_default_sampler(Device& device);

} // namespace gn::rhi
