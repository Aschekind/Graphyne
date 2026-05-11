/**
 * @file rhi/buffer.h
 * @brief Small VkBuffer + VkDeviceMemory pair.
 *
 * Manual allocation per buffer (no VMA yet). Two flavors:
 *   - HostVisible: mappable, used for staging / uniform buffers.
 *   - DeviceLocal: GPU-only, used for static vertex / index buffers.
 *
 * Use `Buffer::upload` to copy CPU data to a device-local buffer via a
 * one-shot staging buffer.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

enum class BufferUsage : u8 {
    Vertex,
    Index,
    Uniform,
    Staging,
};

enum class BufferMemory : u8 {
    HostVisible,    // mappable, coherent
    DeviceLocal,    // GPU-only
};

class Buffer {
public:
    Buffer() = default;
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    Result<void> initialize(Device& device, usize size_bytes, BufferUsage usage, BufferMemory memory);

    void shutdown();

    /// Copy `size` bytes of `data` into this buffer.
    /// HostVisible: memcpy via mapping (persistent or transient).
    /// DeviceLocal: routes through a temporary staging buffer + one-shot copy.
    Result<void> upload(const void* data, usize size, usize offset = 0);

    /// Persistent map (HostVisible only). Returns nullptr on DeviceLocal buffers.
    void* map();
    void  unmap();

    VkBuffer       handle() const { return m_buffer; }
    VkDeviceMemory memory() const { return m_memory; }
    usize          size()   const { return m_size; }

private:
    Device*        m_device  = nullptr;
    VkBuffer       m_buffer  = VK_NULL_HANDLE;
    VkDeviceMemory m_memory  = VK_NULL_HANDLE;
    usize          m_size    = 0;
    BufferMemory   m_storage = BufferMemory::DeviceLocal;
    void*          m_mapped  = nullptr;
};

} // namespace gn::rhi
