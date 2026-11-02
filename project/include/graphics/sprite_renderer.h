/**
 * @file graphics/sprite_renderer.h
 * @brief GPU side of sprite drawing: uploads a finished SpriteBatch and draws it, one draw per range.
 */
#pragma once

#include "core/result.h"
#include "graphics/sprite_batch.h"
#include "rhi/buffer.h"
#include "rhi/pipeline.h"
#include "rhi/shader.h"

#include <array>

namespace gn::rhi {
class Device;
}

namespace gn::graphics {

class TextureManager;

/// Needs shaders/sprite.vert.spv and sprite.frag.spv next to the executable (the library's CMake target builds them).
/// Holds a pointer to the Device: shut it down before the Device.
class SpriteRenderer {
public:
    static constexpr u32 kFramesInFlight = 2;

    SpriteRenderer() = default;
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&)            = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;
    SpriteRenderer(SpriteRenderer&&)                 = delete;
    SpriteRenderer& operator=(SpriteRenderer&&)      = delete;

    /// `color_format` is the swapchain format. `textures` must outlive this object.
    Result<void> initialize(rhi::Device& device, VkFormat color_format, TextureManager& textures);
    void         shutdown();

    /// Record the batch (already finish()ed) into `cmd`, inside a dynamic-rendering pass.
    void record(VkCommandBuffer    cmd,
                u32                frame_index,
                Extent2D           extent,
                const mat4&        view_projection,
                const SpriteBatch& batch);

private:
    TextureManager*                          m_textures = nullptr;
    rhi::Shader                              m_vertex;
    rhi::Shader                              m_fragment;
    rhi::Pipeline                            m_pipeline;
    rhi::Buffer                              m_indices;
    std::array<rhi::Buffer, kFramesInFlight> m_vertex_buffers;
    bool                                     m_logged = false;
};

} // namespace gn::graphics
