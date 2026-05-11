/**
 * @file graphics/sprite_renderer.h
 * @brief Textured-quad pipeline + per-sprite draw submission.
 *
 * Design:
 *   - One graphics pipeline (sprite.vert + sprite.frag).
 *   - One shared quad VBO/IBO (4 vertices, 6 indices) bound for every draw.
 *   - Per-frame camera uniform buffer (set = 0, binding = 0).
 *   - Per-texture combined-image-sampler descriptor set owned by
 *     ResourceManager (set = 1, binding = 0).
 *   - Push constants per draw: mat4 model + vec4 tint.
 *
 * Usage per frame:
 *   sprite_renderer.begin_frame(frame_idx, camera);
 *   sprite_renderer.draw(sprite, *resources.get(sprite.texture));
 *   ...
 *   sprite_renderer.flush(cmd);
 */
#pragma once

#include "core/result.h"
#include "core/types.h"
#include "graphics/camera.h"
#include "graphics/sprite.h"
#include "resources/texture.h"
#include "rhi/buffer.h"

#include <array>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace gn::rhi { class Device; }
namespace gn::resources { class ResourceManager; }

namespace gn::graphics {

class SpriteRenderer {
public:
    static constexpr u32 kMaxFramesInFlight = 2;

    SpriteRenderer() = default;
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&)            = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    /// `color_format` is the swapchain image format the pipeline must target.
    Result<void> initialize(rhi::Device& device,
                            resources::ResourceManager& resources,
                            VkFormat color_format,
                            const std::string& shader_dir);
    void shutdown();

    /// Update per-frame camera UBO; clear the per-frame draw queue.
    void begin_frame(u32 frame_index, const Camera2D& camera);

    /// Queue a sprite for drawing.
    void draw(const Sprite& sprite, const resources::Texture2D& texture);

    /// Encode all queued draws into `cmd`. Must be inside a render pass /
    /// vkCmdBeginRendering scope.
    void flush(VkCommandBuffer cmd);

    /// True once initialize succeeded.
    bool ready() const { return m_pipeline != VK_NULL_HANDLE; }

private:
    Result<void> create_quad_buffers();
    Result<void> create_camera_resources();
    Result<void> create_pipeline(VkFormat color_format, const std::string& shader_dir);

    rhi::Device*                m_device    = nullptr;
    resources::ResourceManager* m_resources = nullptr;

    // Shared quad geometry.
    rhi::Buffer m_quad_vbo;
    rhi::Buffer m_quad_ibo;

    // Per-frame camera resources.
    struct PerFrame {
        rhi::Buffer     ubo;          // host-visible
        VkDescriptorSet camera_set = VK_NULL_HANDLE;
    };
    std::array<PerFrame, kMaxFramesInFlight> m_frames;

    VkDescriptorSetLayout m_camera_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool      m_camera_pool       = VK_NULL_HANDLE;
    VkPipelineLayout      m_pipeline_layout   = VK_NULL_HANDLE;
    VkPipeline            m_pipeline          = VK_NULL_HANDLE;

    // Per-frame draw queue.
    struct PendingDraw {
        mat4            model;
        vec4            tint;
        VkDescriptorSet texture_set;
    };
    std::vector<PendingDraw> m_pending;
    u32                       m_current_frame = 0;
};

} // namespace gn::graphics
