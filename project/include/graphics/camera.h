/**
 * @file graphics/camera.h
 * @brief 2D orthographic camera.
 */
#pragma once

#include "core/types.h"

namespace gn::graphics {

struct Camera2D {
    /// World-space coordinate that maps to the center of the viewport.
    vec2 position {0.0f, 0.0f};
    /// Pixel-to-world scale. 1.0 = 1 unit per pixel.
    f32  zoom = 1.0f;
    /// Viewport size in pixels.
    Extent2D viewport {0, 0};

    /// view * projection matrix mapping world units to clip space.
    /// Origin (0,0) is the top-left of the viewport, +Y is down (screen-space).
    mat4 view_projection() const;
};

} // namespace gn::graphics
