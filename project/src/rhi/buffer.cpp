#include "rhi/buffer.h"

#include "rhi/device.h"
#include "rhi/vma.h"
#include "utils/assert.h"

#include <cstring>
#include <string>

namespace gn::rhi {

Buffer::~Buffer() {
    shutdown();
}

Result<void> Buffer::initialize(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, BufferMemory memory) {
    shutdown();
    if (size == 0) return Err{Error{"Buffer: zero size"}};

    VkBufferCreateInfo bi{};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = size;
    bi.usage       = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO;
    if (memory == BufferMemory::HostVisible) {
        ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocationInfo info{};
    VmaAllocation     allocation = nullptr;
    VkResult          res        = vmaCreateBuffer(device.allocator(), &bi, &ai, &m_buffer, &allocation, &info);
    if (res != VK_SUCCESS) {
        m_buffer = VK_NULL_HANDLE;
        return Err{Error{"vmaCreateBuffer failed (code " + std::to_string(res) + ")"}};
    }
    m_allocation = allocation;
    m_mapped     = info.pMappedData;
    m_size       = size;
    m_device     = &device;
    return Ok();
}

Result<void> Buffer::initialize_device_local(Device&            device,
                                             const void*        data,
                                             VkDeviceSize       size,
                                             VkBufferUsageFlags usage) {
    Buffer staging;
    if (auto r = staging.initialize(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, BufferMemory::HostVisible); !r) {
        return r;
    }
    if (auto r = staging.write(data, size); !r) return r;
    if (auto r = initialize(device, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, BufferMemory::DeviceLocal); !r) {
        return r;
    }

    VkBuffer src = staging.handle();
    VkBuffer dst = m_buffer;
    auto     r   = device.submit_one_shot([&](VkCommandBuffer cmd) {
        VkBufferCopy region{0, 0, size};
        vkCmdCopyBuffer(cmd, src, dst, 1, &region);
    });
    if (!r) shutdown();
    return r;
}

void Buffer::shutdown() {
    if (m_buffer != VK_NULL_HANDLE) {
        GN_ASSERT(m_device->logical_device() != VK_NULL_HANDLE, "shut down after the Device");
        vmaDestroyBuffer(m_device->allocator(), m_buffer, m_allocation);
    }
    m_buffer     = VK_NULL_HANDLE;
    m_allocation = nullptr;
    m_mapped     = nullptr;
    m_size       = 0;
}

Result<void> Buffer::write(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (!m_mapped) return Err{Error{"Buffer::write: buffer is not host-visible"}};
    if (offset + size > m_size) return Err{Error{"Buffer::write: out of range"}};
    std::memcpy(static_cast<char*>(m_mapped) + offset, data, static_cast<size_t>(size));
    // No-op on coherent memory; needed otherwise.
    VkResult res = vmaFlushAllocation(m_device->allocator(), m_allocation, offset, size);
    if (res != VK_SUCCESS) return Err{Error{"vmaFlushAllocation failed (code " + std::to_string(res) + ")"}};
    return Ok();
}

} // namespace gn::rhi
