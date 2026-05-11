/**
 * @file simple_window.cpp
 * @brief Minimal example: open a window, clear the screen, ESC to quit.
 *
 * Demonstrates the App lifecycle. The screen cycles through a soft color
 * gradient so you can visually confirm the renderer is presenting frames.
 */
#include "core/app.h"
#include "core/engine.h"
#include "graphics/frame.h"
#include "platform/event.h"
#include "platform/input.h"
#include "platform/key_codes.h"
#include "utils/logger.h"

#include <cmath>

class DemoApp final : public gn::App {
public:
    bool on_start() override {
        GN_INFO("DemoApp started — press ESC to quit");
        return true;
    }

    void on_update(gn::f32 dt) override {
        m_time += dt;

        // Quit on ESC.
        if (engine()->input().pressed(gn::platform::Key::Escape)) {
            engine()->request_exit();
        }
    }

    void on_render(gn::graphics::Frame& frame) override {
        // Animated clear color.
        const float t = m_time;
        frame.clear_color = gn::Color{
            0.5f + 0.5f * std::sin(t * 0.7f),
            0.5f + 0.5f * std::sin(t * 0.9f + 1.0f),
            0.5f + 0.5f * std::sin(t * 1.1f + 2.0f),
            1.0f,
        };
    }

    void on_resize(gn::u32 w, gn::u32 h) override {
        GN_INFO("Window resized to {}x{}", w, h);
    }

    void on_shutdown() override {
        GN_INFO("DemoApp shutting down");
    }

private:
    float m_time = 0.0f;
};

int main(int /*argc*/, char* /*argv*/[]) {
    gn::Engine::Config cfg;
    cfg.app_name      = "Graphyne — Simple Window";
    cfg.window_width  = 1280;
    cfg.window_height = 720;
    cfg.log_file      = "graphyne.log";

    gn::Engine engine(cfg);
    DemoApp app;
    return engine.run(app);
}
