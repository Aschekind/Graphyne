#include "graphics/camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace gn::graphics {

mat4 Camera2D::view_projection() const {
    const f32 w = viewport.width  > 0 ? static_cast<f32>(viewport.width)  : 1.0f;
    const f32 h = viewport.height > 0 ? static_cast<f32>(viewport.height) : 1.0f;

    // Orthographic projection mapping world units to Vulkan clip space.
    //
    // Vulkan NDC has +Y pointing DOWN (opposite of OpenGL). We want a
    // top-left origin in world space (y=0 at top, y=+h at bottom), so:
    //   - world y=0 must map to NDC y=-1 (top of viewport)    -> bottom=0
    //   - world y=h must map to NDC y=+1 (bottom of viewport) -> top=h
    //
    // glm::ortho was written for OpenGL conventions, so the "obvious"
    // (bottom=h, top=0) call would render upside down on Vulkan.
    mat4 proj = glm::ortho(0.0f, w, 0.0f, h, -1.0f, 1.0f);

    // View: translate by -position, then scale by zoom around the viewport center.
    const vec2 half{w * 0.5f, h * 0.5f};
    mat4 view{1.0f};
    view = glm::translate(view, vec3{half, 0.0f});
    view = glm::scale(view, vec3{zoom, zoom, 1.0f});
    view = glm::translate(view, vec3{-position - half, 0.0f});

    return proj * view;
}

} // namespace gn::graphics
