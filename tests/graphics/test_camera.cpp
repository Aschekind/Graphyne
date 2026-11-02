#include <gtest/gtest.h>

#include <cmath>

#include "graphics/camera.h"

using gn::vec2;
using gn::vec4;
using gn::graphics::Camera2D;

namespace {
vec2 project(const Camera2D& cam, vec2 world) {
    const vec4 clip = cam.view_projection() * vec4{world, 0.0f, 1.0f};
    return {clip.x, clip.y};
}

Camera2D make_camera() {
    Camera2D cam;
    cam.viewport = {800, 600};
    return cam;
}
} // namespace

TEST(Camera2D, PositionMapsToClipCentre) {
    Camera2D cam = make_camera();
    cam.position = {123.0f, -45.0f};
    const vec2 p = project(cam, cam.position);
    EXPECT_NEAR(p.x, 0.0f, 1e-6f);
    EXPECT_NEAR(p.y, 0.0f, 1e-6f);
}

TEST(Camera2D, ViewportCornersAreYDown) {
    Camera2D cam            = make_camera();
    cam.position            = {400.0f, 300.0f}; // world origin at the top-left of the screen
    const vec2 top_left     = project(cam, {0.0f, 0.0f});
    const vec2 bottom_right = project(cam, {800.0f, 600.0f});
    EXPECT_NEAR(top_left.x, -1.0f, 1e-6f);
    EXPECT_NEAR(top_left.y, -1.0f, 1e-6f); // Vulkan clip y = -1 is the top
    EXPECT_NEAR(bottom_right.x, 1.0f, 1e-6f);
    EXPECT_NEAR(bottom_right.y, 1.0f, 1e-6f);
}

TEST(Camera2D, ZoomScalesAroundPosition) {
    Camera2D cam = make_camera();
    cam.zoom     = 2.0f;
    const vec2 p = project(cam, {200.0f, 150.0f});
    EXPECT_NEAR(p.x, 1.0f, 1e-6f);
    EXPECT_NEAR(p.y, 1.0f, 1e-6f);
}

TEST(Camera2D, PositiveRotationTurnsContentCounterClockwise) {
    Camera2D cam = make_camera();
    cam.rotation = glm::half_pi<float>();
    // A point to the right of the camera ends up above the centre (clip y < 0 is up).
    const vec2 p = project(cam, {100.0f, 0.0f});
    EXPECT_NEAR(p.x, 0.0f, 1e-6f);
    EXPECT_NEAR(p.y, -100.0f / 300.0f, 1e-6f);
}

TEST(Camera2D, FollowsViewportSize) {
    Camera2D cam = make_camera();
    cam.viewport = {400, 300};
    const vec2 p = project(cam, {200.0f, 150.0f});
    EXPECT_NEAR(p.x, 1.0f, 1e-6f);
    EXPECT_NEAR(p.y, 1.0f, 1e-6f);
}

TEST(Camera2D, ZeroViewportDoesNotDivideByZero) {
    Camera2D cam;
    cam.viewport     = {0, 0};
    const gn::mat4 m = cam.view_projection();
    EXPECT_TRUE(std::isfinite(m[0][0]));
    EXPECT_TRUE(std::isfinite(m[1][1]));
}
