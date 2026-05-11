/**
 * @file simple_window.cpp
 * @brief Loads a JPEG from examples/assets and draws it as a sprite filling
 *        the window. ESC quits, arrow keys pan, +/- zoom.
 */
#include "core/app.h"
#include "core/engine.h"
#include "graphics/frame.h"
#include "graphics/renderer.h"
#include "graphics/sprite.h"
#include "graphics/sprite_renderer.h"
#include "platform/input.h"
#include "platform/key_codes.h"
#include "resources/resource_manager.h"
#include "utils/logger.h"

#include <cstdio>
#include <exception>
#include <iostream>

class DemoApp final : public gn::App {
public:
    bool on_start() override {
        GN_INFO("DemoApp::on_start — loading background image");

        // Path is resolved relative to the engine's assets dir (configured
        // via GRAPHYNE_ASSETS_DIR at compile time = examples/assets).
        auto result = engine()->resources().load_texture("Road 96 background zoe fire.jpg");
        if (!result) {
            GN_ERROR("Failed to load texture: {}", result.error().what());
            return false;
        }
        m_background = result.value();
        GN_INFO("DemoApp ready. Controls: ESC quit, Arrow keys pan, +/- zoom");
        return true;
    }

    void on_update(gn::f32 dt) override {
        auto& input = engine()->input();
        if (input.pressed(gn::platform::Key::Escape)) {
            engine()->request_exit();
        }

        // Camera panning.
        auto& cam = engine()->renderer().camera();
        const float speed = 600.0f * dt;
        if (input.down(gn::platform::Key::Left))  cam.position.x -= speed;
        if (input.down(gn::platform::Key::Right)) cam.position.x += speed;
        if (input.down(gn::platform::Key::Up))    cam.position.y -= speed;
        if (input.down(gn::platform::Key::Down))  cam.position.y += speed;

        // Camera zoom.
        if (input.down(gn::platform::Key::Num1)) cam.zoom *= (1.0f - 0.8f * dt);
        if (input.down(gn::platform::Key::Num2)) cam.zoom *= (1.0f + 0.8f * dt);
    }

    void on_render(gn::graphics::Frame& frame) override {
        if (!m_background.valid()) return;

        const auto* tex = engine()->resources().get(m_background);
        if (!tex) return;

        // Draw the background sized to the texture's native resolution.
        // The camera's orthographic projection maps pixels to screen 1:1,
        // so this fills the viewport when zoom == 1 and the image is at
        // position (0, 0).
        gn::graphics::Sprite bg;
        bg.texture  = m_background;
        bg.position = {0.0f, 0.0f};
        bg.size     = {static_cast<gn::f32>(tex->width()),
                       static_cast<gn::f32>(tex->height())};

        engine()->renderer().sprites().draw(bg, *tex);

        // Set a black clear color so any letterboxing reads as black.
        frame.clear_color = gn::Color{0.0f, 0.0f, 0.0f, 1.0f};
    }

    void on_resize(gn::u32 w, gn::u32 h) override {
        GN_INFO("Window resized to {}x{}", w, h);
    }

    void on_shutdown() override {
        GN_INFO("DemoApp shutting down");
    }

private:
    gn::TextureHandle m_background;
};

static int run() {
    gn::Engine::Config cfg;
    cfg.app_name      = "Graphyne — Texture Demo";
    cfg.window_width  = 1280;
    cfg.window_height = 720;
    cfg.log_file      = "graphyne.log";

    gn::Engine engine(cfg);
    DemoApp app;
    return engine.run(app);
}

int main(int /*argc*/, char* /*argv*/[]) {
    int code = 1;
    try {
        code = run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Unhandled exception: %s\n", e.what());
    } catch (...) {
        std::fprintf(stderr, "Unhandled non-std exception\n");
    }

    // On Windows the console window closes immediately when launched from
    // Explorer. Pause on error so the user can actually read what went wrong.
#ifdef _WIN32
    if (code != 0) {
        std::fprintf(stderr, "\nExited with code %d. Press Enter to close...\n", code);
        std::cin.get();
    }
#endif
    return code;
}
