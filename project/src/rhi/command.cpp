#include "rhi/command.h"

#include "rhi/device.h"

namespace gn::rhi {

CommandPool::~CommandPool() { shutdown(); }

Result<void> CommandPool::initialize(Device& device, u32 frame_count) {
    m_device = &device;

    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = device.queue_families().graphics;

    VkDevice dev = device.logical_device();
    if (vkCreateCommandPool(dev, &ci, nullptr, &m_pool) != VK_SUCCESS) {
        return Err{Error{"vkCreateCommandPool failed"}};
    }

    m_buffers.resize(frame_count, VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = m_pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = frame_count;
    if (vkAllocateCommandBuffers(dev, &ai, m_buffers.data()) != VK_SUCCESS) {
        return Err{Error{"vkAllocateCommandBuffers failed"}};
    }
    return Ok();
}

void CommandPool::shutdown() {
    if (!m_device) return;
    VkDevice dev = m_device->logical_device();
    if (dev == VK_NULL_HANDLE) {
        m_pool = VK_NULL_HANDLE;
        m_buffers.clear();
        return;
    }
    if (m_pool != VK_NULL_HANDLE) {
        // Command buffers freed implicitly when pool is destroyed.
        vkDestroyCommandPool(dev, m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
    m_buffers.clear();
}

} // namespace gn::rhi
