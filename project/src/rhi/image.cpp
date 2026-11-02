#include "rhi/image.h"

#include "rhi/buffer.h"
#include "rhi/device.h"
#include "rhi/vma.h"
#include "utils/assert.h"

#include <string>

namespace gn::rhi {

namespace {

void transition(VkCommandBuffer       cmd,
                VkImage               image,
                VkImageLayout         from,
                VkImageLayout         to,
                VkPipelineStageFlags2 src_stage,
                VkAccessFlags2        src_access,
                VkPipelineStageFlags2 dst_stage,
                VkAccessFlags2        dst_access) {
    VkImageMemoryBarrier2 b{};
    b.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    b.srcStageMask                = src_stage;
    b.srcAccessMask               = src_access;
    b.dstStageMask                = dst_stage;
    b.dstAccessMask               = dst_access;
    b.oldLayout                   = from;
    b.newLayout                   = to;
    b.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    b.image                       = image;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers    = &b;
    vkCmdPipelineBarrier2(cmd, &dep);
}

} // namespace

Image::~Image() {
    shutdown();
}

Result<void> Image::initialize(Device& device, u32 width, u32 height, VkFormat format) {
    shutdown();
    if (width == 0 || height == 0) return Err{Error{"Image: zero size"}};

    VkImageCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType     = VK_IMAGE_TYPE_2D;
    ci.format        = format;
    ci.extent        = {width, height, 1};
    ci.mipLevels     = 1;
    ci.arrayLayers   = 1;
    ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ci.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO;

    VmaAllocation allocation = nullptr;
    VkResult      res        = vmaCreateImage(device.allocator(), &ci, &ai, &m_image, &allocation, nullptr);
    if (res != VK_SUCCESS) {
        m_image = VK_NULL_HANDLE;
        return Err{Error{"vmaCreateImage failed (code " + std::to_string(res) + ")"}};
    }
    m_allocation = allocation;
    m_device     = &device;
    m_width      = width;
    m_height     = height;

    VkImageViewCreateInfo vi{};
    vi.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image                       = m_image;
    vi.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    vi.format                      = format;
    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.layerCount = 1;
    res                            = vkCreateImageView(device.logical_device(), &vi, nullptr, &m_view);
    if (res != VK_SUCCESS) {
        shutdown();
        return Err{Error{"vkCreateImageView failed (code " + std::to_string(res) + ")"}};
    }
    return Ok();
}

void Image::shutdown() {
    if (m_view != VK_NULL_HANDLE || m_image != VK_NULL_HANDLE)
        GN_ASSERT(m_device->logical_device() != VK_NULL_HANDLE, "shut down after the Device");
    if (m_view != VK_NULL_HANDLE) vkDestroyImageView(m_device->logical_device(), m_view, nullptr);
    if (m_image != VK_NULL_HANDLE) vmaDestroyImage(m_device->allocator(), m_image, m_allocation);
    m_view       = VK_NULL_HANDLE;
    m_image      = VK_NULL_HANDLE;
    m_allocation = nullptr;
    m_width = m_height = 0;
}

Result<void> Image::upload_rgba8(const u8* pixels) {
    if (m_image == VK_NULL_HANDLE) return Err{Error{"Image::upload_rgba8: image not initialized"}};

    const VkDeviceSize size = VkDeviceSize{m_width} * m_height * 4;
    Buffer             staging;
    if (auto r = staging.initialize(*m_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, BufferMemory::HostVisible); !r) {
        return r;
    }
    if (auto r = staging.write(pixels, size); !r) return r;

    return m_device->submit_one_shot([&](VkCommandBuffer cmd) {
        transition(cmd,
                   m_image,
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_PIPELINE_STAGE_2_NONE,
                   VK_ACCESS_2_NONE,
                   VK_PIPELINE_STAGE_2_COPY_BIT,
                   VK_ACCESS_2_TRANSFER_WRITE_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent                 = {m_width, m_height, 1};
        vkCmdCopyBufferToImage(cmd, staging.handle(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        transition(cmd,
                   m_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   VK_PIPELINE_STAGE_2_COPY_BIT,
                   VK_ACCESS_2_TRANSFER_WRITE_BIT,
                   VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                   VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    });
}

Sampler::~Sampler() {
    shutdown();
}

Result<void> Sampler::initialize(Device& device, Filter filter) {
    shutdown();
    const VkFilter vk_filter = filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;

    VkSamplerCreateInfo si{};
    si.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter    = vk_filter;
    si.minFilter    = vk_filter;
    si.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.borderColor                                      = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    VkResult res = vkCreateSampler(device.logical_device(), &si, nullptr, &m_sampler);
    if (res != VK_SUCCESS) return Err{Error{"vkCreateSampler failed (code " + std::to_string(res) + ")"}};
    m_device = &device;
    return Ok();
}

void Sampler::shutdown() {
    if (m_sampler != VK_NULL_HANDLE)
        GN_ASSERT(m_device->logical_device() != VK_NULL_HANDLE, "shut down after the Device");
    if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(m_device->logical_device(), m_sampler, nullptr);
    m_sampler = VK_NULL_HANDLE;
}

} // namespace gn::rhi
