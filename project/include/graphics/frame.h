/**
 * @file graphics/frame.h
 * @brief Per-frame context passed to App::on_render.
 *
 * Holds the in-flight command buffer, the target swapchain image view,
 * the rendering extent, and frame metadata. Game code mutates the frame
 * via the high-level Renderer (e.g. SpriteRenderer) — it does not touch
 * Vulkan directly.
 */
#pragma once

#include "core/types.h"
#include "graphics/color.h"

#include <vulkan/vulkan.h>

namespace gn::graphics {

class Frame {
public:
    /// Index of the current frame-in-flight (0..MAX_FRAMES_IN_FLIGHT-1).
    u32 frame_index = 0;
    /// Index of the swapchain image being rendered to.
    u32 image_index = 0;
    /// Wall-clock seconds since the engine started.
    f64 time = 0.0;
    /// Seconds since the previous frame.
    f32 delta = 0.0f;

    /// Color used by the renderer to clear the swapchain at the start of the
    /// frame. Game code can mutate this before any draw calls.
    Color clear_color = colors::CornflowerBlue;

    /// Active command buffer for this frame. Set by Renderer::begin_frame.
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    /// Target swapchain image view (color attachment).
    VkImageView target_view = VK_NULL_HANDLE;
    VkImage     target_image = VK_NULL_HANDLE;
    Extent2D    target_extent = {0, 0};
};

} // namespace gn::graphics
