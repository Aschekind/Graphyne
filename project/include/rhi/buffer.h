/**
 * @file rhi/buffer.h
 * @brief VkBuffer backed by a VMA allocation.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <vulkan/vulkan.h>

struct VmaAllocation_T; // opaque VMA handle; keeps vk_mem_alloc.h out of public headers

namespace gn::rhi {

class Device;

enum class BufferMemory {
    HostVisible, ///< CPU-writable and persistently mapped (staging, per-frame data).
    DeviceLocal, ///< GPU memory; fill it through initialize_device_local().
};

/// VMA-backed VkBuffer. /// Holds a pointer to the Device: shut it down (or destroy it) before the Device.
class Buffer {
public:
    Buffer() = default;
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&)                 = delete;
    Buffer& operator=(Buffer&&)      = delete;

    Result<void> initialize(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, BufferMemory memory);

    /// Create a device-local buffer (usage | TRANSFER_DST) holding `data`, through a staging buffer.
    /// Blocks until the copy has finished.
    Result<void> initialize_device_local(Device& device, const void* data, VkDeviceSize size, VkBufferUsageFlags usage);

    void shutdown();

    /// Copy `size` bytes to `offset`. Host-visible buffers only.
    Result<void> write(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    VkBuffer     handle() const { return m_buffer; }
    VkDeviceSize size() const { return m_size; }

private:
    Device*          m_device     = nullptr;
    VkBuffer         m_buffer     = VK_NULL_HANDLE;
    VmaAllocation_T* m_allocation = nullptr;
    void*            m_mapped     = nullptr;
    VkDeviceSize     m_size       = 0;
};

} // namespace gn::rhi
