#include "rhi/buffer.h"

#include "rhi/device.h"
#include "utils/logger.h"

#include <cstring>

namespace gn::rhi {

namespace {

VkBufferUsageFlags translate_usage(BufferUsage u, bool include_transfer_dst) {
    VkBufferUsageFlags flags = 0;
    switch (u) {
        case BufferUsage::Vertex:  flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
        case BufferUsage::Index:   flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;  break;
        case BufferUsage::Uniform: flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;break;
        case BufferUsage::Staging: flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  break;
    }
    if (include_transfer_dst) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return flags;
}

VkMemoryPropertyFlags translate_memory(BufferMemory m) {
    switch (m) {
        case BufferMemory::HostVisible:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        case BufferMemory::DeviceLocal:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    return 0;
}

} // namespace

Buffer::~Buffer() { shutdown(); }

Buffer::Buffer(Buffer&& other) noexcept
    : m_device(other.m_device), m_buffer(other.m_buffer), m_memory(other.m_memory),
      m_size(other.m_size), m_storage(other.m_storage), m_mapped(other.m_mapped) {
    other.m_device = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size   = 0;
    other.m_mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        shutdown();
        m_device  = other.m_device;
        m_buffer  = other.m_buffer;
        m_memory  = other.m_memory;
        m_size    = other.m_size;
        m_storage = other.m_storage;
        m_mapped  = other.m_mapped;
        other.m_device = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size   = 0;
        other.m_mapped = nullptr;
    }
    return *this;
}

Result<void> Buffer::initialize(Device& device, usize size_bytes, BufferUsage usage, BufferMemory memory) {
    if (size_bytes == 0) return Err{Error{"Buffer::initialize size must be > 0"}};
    m_device  = &device;
    m_size    = size_bytes;
    m_storage = memory;

    VkBufferCreateInfo bi{};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = size_bytes;
    bi.usage       = translate_usage(usage, /*include_transfer_dst=*/memory == BufferMemory::DeviceLocal);
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice dev = device.logical_device();
    if (vkCreateBuffer(dev, &bi, nullptr, &m_buffer) != VK_SUCCESS) {
        return Err{Error{"vkCreateBuffer failed"}};
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(dev, m_buffer, &req);

    const u32 mem_type = device.find_memory_type(req.memoryTypeBits, translate_memory(memory));
    if (mem_type == ~0u) {
        return Err{Error{"No suitable memory type for buffer"}};
    }

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = mem_type;
    if (vkAllocateMemory(dev, &ai, nullptr, &m_memory) != VK_SUCCESS) {
        return Err{Error{"vkAllocateMemory (buffer) failed"}};
    }
    vkBindBufferMemory(dev, m_buffer, m_memory, 0);
    return Ok();
}

void Buffer::shutdown() {
    if (!m_device) return;
    VkDevice dev = m_device->logical_device();
    if (dev == VK_NULL_HANDLE) {
        m_buffer = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
        m_mapped = nullptr;
        m_device = nullptr;
        return;
    }
    if (m_mapped) {
        vkUnmapMemory(dev, m_memory);
        m_mapped = nullptr;
    }
    if (m_buffer) vkDestroyBuffer(dev, m_buffer, nullptr);
    if (m_memory) vkFreeMemory(dev, m_memory, nullptr);
    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_device = nullptr;
    m_size   = 0;
}

void* Buffer::map() {
    if (!m_device || m_storage != BufferMemory::HostVisible) return nullptr;
    if (m_mapped) return m_mapped;
    if (vkMapMemory(m_device->logical_device(), m_memory, 0, m_size, 0, &m_mapped) != VK_SUCCESS) {
        m_mapped = nullptr;
    }
    return m_mapped;
}

void Buffer::unmap() {
    if (m_device && m_mapped) {
        vkUnmapMemory(m_device->logical_device(), m_memory);
        m_mapped = nullptr;
    }
}

Result<void> Buffer::upload(const void* data, usize size, usize offset) {
    if (!m_device || size == 0 || offset + size > m_size) {
        return Err{Error{"Buffer::upload bad arguments"}};
    }

    if (m_storage == BufferMemory::HostVisible) {
        void* dst = map();
        if (!dst) return Err{Error{"vkMapMemory failed"}};
        std::memcpy(static_cast<u8*>(dst) + offset, data, size);
        // Coherent memory does not need flushing.
        return Ok();
    }

    // DeviceLocal: stage through a temporary HostVisible buffer.
    Buffer staging;
    if (auto r = staging.initialize(*m_device, size, BufferUsage::Staging, BufferMemory::HostVisible); !r) return r;
    if (auto r = staging.upload(data, size, 0); !r) return r;

    m_device->submit_one_shot([&](VkCommandBuffer cmd) {
        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = offset;
        region.size      = size;
        vkCmdCopyBuffer(cmd, staging.handle(), m_buffer, 1, &region);
    });
    return Ok();
}

} // namespace gn::rhi
