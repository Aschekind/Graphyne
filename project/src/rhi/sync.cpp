#include "rhi/sync.h"

#include "rhi/device.h"

namespace gn::rhi {

bool create_frame_sync(Device& device, FrameSync& out) {
    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // first wait returns immediately

    VkDevice dev = device.logical_device();
    if (vkCreateSemaphore(dev, &si, nullptr, &out.image_available) != VK_SUCCESS) return false;
    if (vkCreateSemaphore(dev, &si, nullptr, &out.render_finished) != VK_SUCCESS) return false;
    if (vkCreateFence(dev,     &fi, nullptr, &out.in_flight)       != VK_SUCCESS) return false;
    return true;
}

void destroy_frame_sync(Device& device, FrameSync& sync) {
    VkDevice dev = device.logical_device();
    if (sync.image_available != VK_NULL_HANDLE) vkDestroySemaphore(dev, sync.image_available, nullptr);
    if (sync.render_finished != VK_NULL_HANDLE) vkDestroySemaphore(dev, sync.render_finished, nullptr);
    if (sync.in_flight       != VK_NULL_HANDLE) vkDestroyFence(dev,     sync.in_flight,       nullptr);
    sync = {};
}

} // namespace gn::rhi
