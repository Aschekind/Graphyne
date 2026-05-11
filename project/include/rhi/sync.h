/**
 * @file rhi/sync.h
 * @brief Frame synchronization primitives (semaphores + fences).
 *
 * Owns the GPU-side sync objects for in-flight frames. The Renderer holds
 * one FrameSync per frame-in-flight.
 */
#pragma once

#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

struct FrameSync {
    VkSemaphore image_available = VK_NULL_HANDLE;
    VkSemaphore render_finished = VK_NULL_HANDLE;
    VkFence     in_flight       = VK_NULL_HANDLE;
};

bool create_frame_sync(Device& device, FrameSync& out);
void destroy_frame_sync(Device& device, FrameSync& sync);

} // namespace gn::rhi
