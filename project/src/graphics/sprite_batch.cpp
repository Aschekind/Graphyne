#include "graphics/sprite_batch.h"

#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace gn::graphics {

void SpriteBatch::draw(const Sprite& s) {
    if (m_meta.size() >= kMaxSprites) {
        if (m_dropped++ == 0) GN_WARNING("SpriteBatch full ({} sprites): dropping the rest of the frame", kMaxSprites);
        return;
    }
    if (!m_meta.empty() && s.layer < m_meta.back().layer) m_needs_sort = true;
    m_meta.push_back({s.texture, s.layer});

    const f32  cs = std::cos(s.rotation);
    const f32  sn = std::sin(s.rotation);
    const vec4 color{s.tint.r, s.tint.g, s.tint.b, s.tint.a};
    for (u32 i = 0; i < 4; ++i) {
        const vec2 corner{static_cast<f32>(i & 1), static_cast<f32>(i >> 1)};
        const vec2 local = (corner - s.pivot) * s.size;
        const vec2 pos   = s.position + vec2{cs * local.x - sn * local.y, sn * local.x + cs * local.y};
        const vec2 uv{corner.x == 0.0f ? s.uv.x : s.uv.z, corner.y == 0.0f ? s.uv.y : s.uv.w};
        m_vertices.push_back({pos, uv, color});
    }
}

void SpriteBatch::finish() {
    if (m_needs_sort) {
        std::vector<u32> order(m_meta.size());
        std::iota(order.begin(), order.end(), 0u);
        std::stable_sort(order.begin(), order.end(), [&](u32 a, u32 b) { return m_meta[a].layer < m_meta[b].layer; });
        std::vector<SpriteVertex> vertices;
        std::vector<Meta>         meta;
        vertices.reserve(m_vertices.size());
        meta.reserve(m_meta.size());
        for (u32 i : order) {
            vertices.insert(vertices.end(), m_vertices.begin() + 4 * i, m_vertices.begin() + 4 * i + 4);
            meta.push_back(m_meta[i]);
        }
        m_vertices   = std::move(vertices);
        m_meta       = std::move(meta);
        m_needs_sort = false;
    }

    m_ranges.clear();
    for (u32 i = 0; i < m_meta.size(); ++i) {
        if (!m_ranges.empty() && m_ranges.back().texture == m_meta[i].texture) {
            m_ranges.back().index_count += 6;
        } else {
            m_ranges.push_back({m_meta[i].texture, 6 * i, 6});
        }
    }
}

void SpriteBatch::clear() {
    m_vertices.clear();
    m_meta.clear();
    m_ranges.clear();
    m_needs_sort = false;
    m_dropped    = 0;
}

} // namespace gn::graphics
