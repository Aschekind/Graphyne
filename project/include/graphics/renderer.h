/**
 * @file graphics/renderer.h
 * @brief High-level renderer that drives the RHI.
 *
 * Owns the per-frame command pool and sync primitives. Game code
 * never touches Vulkan directly — it gets a Frame in on_render and
 * (eventually) draws sprites/meshes through the renderer's APIs.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"
#include "graphics/color.h"
#include "graphics/frame.h"
#include "rhi/command.h"
#include "rhi/sync.h"

#include <array>
#include <memory>

namespace gn::rhi      { class Device; class Swapchain; }
namespace gn::platform { class Window; }

namespace gn::graphics {

class Renderer {
public:
    static constexpr u32 kMaxFramesInFlight = 2;

    struct Config {
        bool vsync = true;
    };

    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&)                 = delete;
    Renderer& operator=(Renderer&&)      = delete;

    Result<void> initialize(rhi::Device& device,
                            rhi::Swapchain& swapchain,
                            platform::Window& window,
                            const Config& cfg = {});

    void shutdown();

    /// Begin a new frame. Returns a Frame ready for the App to populate,
    /// or nullptr if the swapchain needs recreation (e.g. minimized window).
    Frame* begin_frame(f64 time, f32 delta);

    /// Finish recording and submit the frame.
    void end_frame();

    /// Notify the renderer that the window size changed. The next begin_frame
    /// will recreate the swapchain.
    void on_resize();

    Color& clear_color() { return m_current_frame.clear_color; }

private:
    void   transition_image(VkCommandBuffer cmd, VkImage img,
                            VkImageLayout old_layout, VkImageLayout new_layout,
                            VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                            VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access);
    Result<void> recreate_swapchain();

    rhi::Device*       m_device    = nullptr;
    rhi::Swapchain*    m_swapchain = nullptr;
    platform::Window*  m_window    = nullptr;

    rhi::CommandPool   m_command_pool;
    std::array<rhi::FrameSync, kMaxFramesInFlight> m_sync;

    Config m_config;
    u32    m_frame_index           = 0;
    u32    m_acquired_image_index  = 0;
    bool   m_needs_recreate        = false;
    bool   m_frame_in_progress     = false;

    Frame  m_current_frame;
};

} // namespace gn::graphics
