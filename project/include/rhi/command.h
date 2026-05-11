/**
 * @file rhi/command.h
 * @brief Command pool and per-frame command buffer wrapper.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

class CommandPool {
public:
    CommandPool() = default;
    ~CommandPool();

    CommandPool(const CommandPool&)            = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&&)                 = delete;
    CommandPool& operator=(CommandPool&&)      = delete;

    /// Create a pool on the device's graphics queue family. Allocates
    /// `frame_count` primary command buffers up front.
    Result<void> initialize(Device& device, u32 frame_count);

    void shutdown();

    VkCommandBuffer buffer(u32 frame_index) const { return m_buffers[frame_index]; }
    VkCommandPool   pool()                  const { return m_pool; }

private:
    Device*                       m_device = nullptr;
    VkCommandPool                 m_pool   = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer>  m_buffers;
};

} // namespace gn::rhi
