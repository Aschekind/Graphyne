/**
 * @file graphics/camera.h
 * @brief 2D camera: world space (pixel units, +x right, +y down) to Vulkan clip space.
 */
#pragma once

#include "core/types.h"

#include <algorithm>

namespace gn::graphics {

/// World units are pixels at zoom 1. Screen space is y-down like Vulkan's clip space, so the
/// matrix has no y flip: the top of the viewport maps to clip y = -1.
struct Camera2D {
    /// World point shown at the centre of the viewport.
    vec2 position{0.0f, 0.0f};
    /// Pixels per world unit (2 shows everything twice as big).
    f32 zoom = 1.0f;
    /// Radians. A positive angle turns the view clockwise, so the content turns counter-clockwise.
    f32 rotation = 0.0f;
    /// Render target size in pixels; the Renderer sets it to the swapchain extent every frame.
    Extent2D viewport{1, 1};

    mat4 view_projection() const {
        const f32 w = static_cast<f32>(std::max(viewport.width, 1u));
        const f32 h = static_cast<f32>(std::max(viewport.height, 1u));
        mat4      m = glm::scale(mat4{1.0f}, vec3{2.0f / w, 2.0f / h, 1.0f});
        m           = glm::scale(m, vec3{zoom, zoom, 1.0f});
        m           = glm::rotate(m, -rotation, vec3{0.0f, 0.0f, 1.0f});
        return glm::translate(m, vec3{-position, 0.0f});
    }
};

} // namespace gn::graphics
