#include "graphics/sprite_renderer.h"

#include "graphics/texture_manager.h"
#include "platform/platform.h"
#include "utils/logger.h"

#include <cstddef>
#include <vector>

namespace gn::graphics {

SpriteRenderer::~SpriteRenderer() {
    shutdown();
}

Result<void> SpriteRenderer::initialize(rhi::Device& device, VkFormat color_format, TextureManager& textures) {
    shutdown();
    m_textures = &textures;

    const std::string shaders = platform::base_path() + "shaders/";
    if (auto r = m_vertex.load(device, shaders + "sprite.vert.spv"); !r) return r;
    if (auto r = m_fragment.load(device, shaders + "sprite.frag.spv"); !r) return r;

    // The same quad pattern for every sprite; u16 is enough because kMaxSprites * 4 <= 65536.
    std::vector<u16> indices;
    indices.reserve(SpriteBatch::kMaxSprites * 6);
    for (u32 i = 0; i < SpriteBatch::kMaxSprites; ++i) {
        const u16 v = static_cast<u16>(i * 4);
        indices.insert(indices.end(),
                       {v,
                        static_cast<u16>(v + 1),
                        static_cast<u16>(v + 2),
                        static_cast<u16>(v + 2),
                        static_cast<u16>(v + 1),
                        static_cast<u16>(v + 3)});
    }
    if (auto r = m_indices.initialize_device_local(
            device, indices.data(), indices.size() * sizeof(u16), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        !r) {
        return r;
    }
    for (auto& b : m_vertex_buffers) {
        if (auto r = b.initialize(device,
                                  SpriteBatch::kMaxSprites * 4 * sizeof(SpriteVertex),
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  rhi::BufferMemory::HostVisible);
            !r) {
            return r;
        }
    }

    rhi::PipelineDesc desc;
    desc.vertex            = m_vertex.module();
    desc.fragment          = m_fragment.module();
    desc.color_format      = color_format;
    desc.alpha_blend       = true;
    desc.set_layouts       = {textures.set_layout()};
    desc.push_constants    = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4)}};
    desc.vertex_bindings   = {{0, sizeof(SpriteVertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    desc.vertex_attributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpriteVertex, pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpriteVertex, uv)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SpriteVertex, color)},
    };
    return m_pipeline.initialize(device, desc);
}

void SpriteRenderer::shutdown() {
    m_pipeline.shutdown();
    for (auto& b : m_vertex_buffers) b.shutdown();
    m_indices.shutdown();
    m_fragment.shutdown();
    m_vertex.shutdown();
    m_textures = nullptr;
}

void SpriteRenderer::record(VkCommandBuffer    cmd,
                            u32                frame_index,
                            Extent2D           extent,
                            const mat4&        view_projection,
                            const SpriteBatch& batch) {
    if (batch.ranges().empty() || !m_textures) return;

    rhi::Buffer& vb = m_vertex_buffers[frame_index % kFramesInFlight];
    if (auto r = vb.write(batch.vertices().data(), batch.vertices().size() * sizeof(SpriteVertex)); !r) {
        GN_ERROR("Sprite upload failed: {}", r.error().what());
        return;
    }

    const VkViewport viewport{0.0f, 0.0f, static_cast<f32>(extent.width), static_cast<f32>(extent.height), 0.0f, 1.0f};
    const VkRect2D   scissor{{0, 0}, {extent.width, extent.height}};
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle());
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdPushConstants(cmd, m_pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &view_projection);
    const VkBuffer     buffer = vb.handle();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_indices.handle(), 0, VK_INDEX_TYPE_UINT16);

    u32 draws = 0;
    for (const DrawRange& range : batch.ranges()) {
        VkDescriptorSet set = m_textures->descriptor(range.texture);
        if (set == VK_NULL_HANDLE) continue; // destroyed or never created texture
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(), 0, 1, &set, 0, nullptr);
        vkCmdDrawIndexed(cmd, range.index_count, 1, range.first_index, 0, 0);
        ++draws;
    }
    if (!m_logged) {
        m_logged = true;
        GN_DEBUG("First sprite frame: {} sprites, {} draw calls", batch.sprite_count(), draws);
    }
}

} // namespace gn::graphics
