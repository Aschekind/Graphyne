/**
 * @file resources/texture.h
 * @brief Texture2D resource — VkImage + sampler + descriptor set.
 *
 * A Texture2D owns its GPU image and a single combined-image-sampler
 * descriptor set (set = 1, binding = 0 of the sprite pipeline layout).
 *
 * Textures are NOT constructed directly by user code; they are created
 * and owned by the ResourceManager.
 */
#pragma once

#include "core/types.h"
#include "rhi/image.h"

#include <vulkan/vulkan.h>

namespace gn::resources {

struct Texture2D {
    rhi::Image       image;
    VkSampler        sampler        = VK_NULL_HANDLE;  // owned by ResourceManager (shared)
    VkDescriptorSet  descriptor_set = VK_NULL_HANDLE;  // owned by ResourceManager (pool-allocated)

    u32 width()  const { return image.width(); }
    u32 height() const { return image.height(); }
};

} // namespace gn::resources
