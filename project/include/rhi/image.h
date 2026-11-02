/**
 * @file rhi/image.h
 * @brief 2D sampled image (with view) and sampler.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <vulkan/vulkan.h>

struct VmaAllocation_T; // opaque VMA handle; keeps vk_mem_alloc.h out of public headers

namespace gn::rhi {

class Device;

/// RGBA8 image usable as a texture (SAMPLED | TRANSFER_DST).
/// /// Holds a pointer to the Device: shut it down (or destroy it) before the Device.
class Image {
public:
    Image() = default;
    ~Image();

    Image(const Image&)            = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&)                 = delete;
    Image& operator=(Image&&)      = delete;

    /// `format` is VK_FORMAT_R8G8B8A8_SRGB for colour data, _UNORM for data textures.
    Result<void> initialize(Device& device, u32 width, u32 height, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    void         shutdown();

    /// Upload width*height*4 bytes through a staging buffer and leave the image in SHADER_READ_ONLY_OPTIMAL.
    /// Blocks until done. Call once per image (the old contents are discarded).
    Result<void> upload_rgba8(const u8* pixels);

    VkImage     handle() const { return m_image; }
    VkImageView view() const { return m_view; }
    u32         width() const { return m_width; }
    u32         height() const { return m_height; }

private:
    Device*          m_device     = nullptr;
    VkImage          m_image      = VK_NULL_HANDLE;
    VkImageView      m_view       = VK_NULL_HANDLE;
    VmaAllocation_T* m_allocation = nullptr;
    u32              m_width      = 0;
    u32              m_height     = 0;
};

/// VkSampler. /// Holds a pointer to the Device: shut it down (or destroy it) before the Device.
class Sampler {
public:
    enum class Filter { Nearest, Linear };

    Sampler() = default;
    ~Sampler();

    Sampler(const Sampler&)            = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler(Sampler&&)                 = delete;
    Sampler& operator=(Sampler&&)      = delete;

    /// Clamp-to-edge, no mipmaps.
    Result<void> initialize(Device& device, Filter filter);
    void         shutdown();

    VkSampler handle() const { return m_sampler; }

private:
    Device*   m_device  = nullptr;
    VkSampler m_sampler = VK_NULL_HANDLE;
};

} // namespace gn::rhi
