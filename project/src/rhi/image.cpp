#include "rhi/image.h"

#include "rhi/buffer.h"
#include "rhi/device.h"
#include "utils/logger.h"

namespace gn::rhi {

namespace {

void cmd_transition(VkCommandBuffer cmd, VkImage img,
                    VkImageLayout old_l, VkImageLayout new_l,
                    VkPipelineStageFlags src_stage, VkAccessFlags src_access,
                    VkPipelineStageFlags dst_stage, VkAccessFlags dst_access) {
    VkImageMemoryBarrier b{};
    b.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.srcAccessMask    = src_access;
    b.dstAccessMask    = dst_access;
    b.oldLayout        = old_l;
    b.newLayout        = new_l;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image            = img;
    b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.baseMipLevel   = 0;
    b.subresourceRange.levelCount     = 1;
    b.subresourceRange.baseArrayLayer = 0;
    b.subresourceRange.layerCount     = 1;

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0,
                         0, nullptr, 0, nullptr, 1, &b);
}

} // namespace

Image::~Image() { shutdown(); }

Image::Image(Image&& other) noexcept
    : m_device(other.m_device), m_image(other.m_image), m_view(other.m_view),
      m_memory(other.m_memory), m_desc(other.m_desc), m_layout(other.m_layout) {
    other.m_device = nullptr;
    other.m_image  = VK_NULL_HANDLE;
    other.m_view   = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_desc   = {};
    other.m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        shutdown();
        m_device = other.m_device;
        m_image  = other.m_image;
        m_view   = other.m_view;
        m_memory = other.m_memory;
        m_desc   = other.m_desc;
        m_layout = other.m_layout;
        other.m_device = nullptr;
        other.m_image  = VK_NULL_HANDLE;
        other.m_view   = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_desc   = {};
        other.m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    return *this;
}

Result<void> Image::initialize(Device& device, const ImageDesc& desc) {
    if (desc.width == 0 || desc.height == 0) return Err{Error{"Image: zero dimensions"}};
    m_device = &device;
    m_desc   = desc;
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType     = VK_IMAGE_TYPE_2D;
    ci.format        = desc.format;
    ci.extent        = {desc.width, desc.height, 1};
    ci.mipLevels     = 1;
    ci.arrayLayers   = 1;
    ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ci.usage         = desc.usage;
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDevice dev = device.logical_device();
    if (vkCreateImage(dev, &ci, nullptr, &m_image) != VK_SUCCESS) {
        return Err{Error{"vkCreateImage failed"}};
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(dev, m_image, &req);
    const u32 mem_type = device.find_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == ~0u) return Err{Error{"No DEVICE_LOCAL memory type for image"}};

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = mem_type;
    if (vkAllocateMemory(dev, &ai, nullptr, &m_memory) != VK_SUCCESS) {
        return Err{Error{"vkAllocateMemory (image) failed"}};
    }
    vkBindImageMemory(dev, m_image, m_memory, 0);

    VkImageViewCreateInfo vci{};
    vci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.image    = m_image;
    vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vci.format   = desc.format;
    vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = 1;
    if (vkCreateImageView(dev, &vci, nullptr, &m_view) != VK_SUCCESS) {
        return Err{Error{"vkCreateImageView failed"}};
    }
    return Ok();
}

void Image::shutdown() {
    if (!m_device) return;
    VkDevice dev = m_device->logical_device();
    if (dev == VK_NULL_HANDLE) {
        m_image = VK_NULL_HANDLE;
        m_view  = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
        m_device = nullptr;
        return;
    }
    if (m_view)   vkDestroyImageView(dev, m_view, nullptr);
    if (m_image)  vkDestroyImage(dev, m_image, nullptr);
    if (m_memory) vkFreeMemory(dev, m_memory, nullptr);
    m_view   = VK_NULL_HANDLE;
    m_image  = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_device = nullptr;
}

Result<void> Image::upload_pixels(std::span<const u8> pixels, VkImageLayout final_layout) {
    if (!m_device) return Err{Error{"Image not initialized"}};
    if (pixels.empty()) return Err{Error{"upload_pixels: empty data"}};

    Buffer staging;
    if (auto r = staging.initialize(*m_device, pixels.size(), BufferUsage::Staging, BufferMemory::HostVisible); !r) return r;
    if (auto r = staging.upload(pixels.data(), pixels.size(), 0); !r) return r;

    m_device->submit_one_shot([&](VkCommandBuffer cmd) {
        cmd_transition(cmd, m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
            VK_PIPELINE_STAGE_TRANSFER_BIT,    VK_ACCESS_TRANSFER_WRITE_BIT);

        VkBufferImageCopy copy{};
        copy.bufferOffset = 0;
        copy.bufferRowLength   = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel       = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount     = 1;
        copy.imageOffset = {0, 0, 0};
        copy.imageExtent = {m_desc.width, m_desc.height, 1};
        vkCmdCopyBufferToImage(cmd, staging.handle(), m_image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        cmd_transition(cmd, m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, final_layout,
            VK_PIPELINE_STAGE_TRANSFER_BIT,        VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
    });

    m_layout = final_layout;
    return Ok();
}

VkSampler create_default_sampler(Device& device) {
    VkSampler sampler = VK_NULL_HANDLE;

    VkSamplerCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter     = VK_FILTER_LINEAR;
    ci.minFilter     = VK_FILTER_LINEAR;
    ci.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.anisotropyEnable = VK_FALSE;
    ci.maxAnisotropy = 1.0f;
    ci.borderColor   = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    ci.unnormalizedCoordinates = VK_FALSE;
    ci.compareEnable = VK_FALSE;
    ci.compareOp     = VK_COMPARE_OP_ALWAYS;
    ci.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.mipLodBias    = 0.0f;
    ci.minLod        = 0.0f;
    ci.maxLod        = 0.0f;

    vkCreateSampler(device.logical_device(), &ci, nullptr, &sampler);
    return sampler;
}

} // namespace gn::rhi
