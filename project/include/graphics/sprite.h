/**
 * @file graphics/sprite.h
 * @brief Sprite — a 2D textured quad in world space.
 */
#pragma once

#include "core/handle.h"
#include "core/types.h"

namespace gn::graphics {

struct Sprite {
    TextureHandle texture;
    /// Top-left in world space.
    vec2 position {0.0f, 0.0f};
    /// Size in world units (pixels by default).
    vec2 size     {1.0f, 1.0f};
    /// Origin of rotation as a fraction of size. (0.5, 0.5) = center.
    vec2 origin   {0.0f, 0.0f};
    /// Rotation in radians.
    f32  rotation = 0.0f;
    /// Multiplicative color tint (white = pass-through).
    vec4 tint     {1.0f, 1.0f, 1.0f, 1.0f};
};

} // namespace gn::graphics
