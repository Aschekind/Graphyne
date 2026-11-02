/**
 * @file graphics/sprite_batch.h
 * @brief CPU side of sprite drawing: turns draw calls into quads and per-texture draw ranges. No Vulkan.
 */
#pragma once

#include "core/handle.h"
#include "core/types.h"
#include "graphics/color.h"

#include <vector>

namespace gn::graphics {

struct SpriteVertex {
    vec2 pos;
    vec2 uv;
    vec4 color;
};

struct Sprite {
    TextureHandle texture;
    vec2          position{0.0f, 0.0f};       ///< world position of the pivot
    vec2          size{1.0f, 1.0f};           ///< world units (pixels at zoom 1)
    f32           rotation = 0.0f;            ///< radians around the pivot; clockwise on screen (y-down)
    vec2          pivot{0.0f, 0.0f};          ///< 0..1 inside the sprite; (0,0) top-left, (0.5,0.5) centre
    vec4          uv{0.0f, 0.0f, 1.0f, 1.0f}; ///< u0, v0, u1, v1 of the top-left and bottom-right corners
    Color         tint  = colors::White;
    i32           layer = 0; ///< higher draws later (on top); equal layers keep submission order
};

/// A run of indices that share one texture.
struct DrawRange {
    TextureHandle texture;
    u32           first_index = 0;
    u32           index_count = 0;
};

/// Collect with draw(), call finish() once, read vertices() and ranges(), then clear() for the next frame.
/// Vertices are 4 per sprite (top-left, top-right, bottom-left, bottom-right); sprite i uses vertices 4i..4i+3
/// and indices 6i..6i+5 of the fixed pattern 0,1,2, 2,1,3.
class SpriteBatch {
public:
    static constexpr u32 kMaxSprites = 16384; ///< 65536 vertices: indices fit in u16

    /// Drops the sprite (and warns once per frame) beyond kMaxSprites.
    void draw(const Sprite& sprite);

    /// Orders by layer (stable, only if needed) and builds the ranges.
    void finish();
    void clear();

    const std::vector<SpriteVertex>& vertices() const { return m_vertices; }
    const std::vector<DrawRange>&    ranges() const { return m_ranges; }
    u32                              sprite_count() const { return static_cast<u32>(m_meta.size()); }
    u32                              dropped() const { return m_dropped; }

private:
    struct Meta {
        TextureHandle texture;
        i32           layer;
    };
    std::vector<SpriteVertex> m_vertices;
    std::vector<Meta>         m_meta;
    std::vector<DrawRange>    m_ranges;
    bool                      m_needs_sort = false;
    u32                       m_dropped    = 0;
};

} // namespace gn::graphics
