/**
 * @file simple_window.cpp
 * @brief Sprite example: a few hundred bouncing, spinning, tinted sprites from three generated textures.
 *
 * ESC quits. Pass an image path (PNG, JPEG, ...) to add a few sprites that use it.
 * Shows the app-facing API only: Engine::textures() to make textures, Engine::sprites() to draw,
 * Engine::camera() for the view. No Vulkan in here.
 */
#include "core/app.h"
#include "core/engine.h"
#include "graphics/camera.h"
#include "graphics/checkerboard.h"
#include "graphics/frame.h"
#include "graphics/sprite_batch.h"
#include "graphics/texture_manager.h"
#include "platform/event.h"
#include "platform/input.h"
#include "platform/key_codes.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Mover {
    gn::TextureHandle texture;
    gn::vec2          position;
    gn::vec2          velocity;
    gn::vec2          size;
    gn::f32           angle;
    gn::f32           spin;
    gn::Color         tint;
    gn::i32           layer;
};

} // namespace

class DemoApp final : public gn::App {
public:
    explicit DemoApp(std::string image_path) : m_image_path(std::move(image_path)) {}

    bool on_start() override {
        GN_INFO("DemoApp started — press ESC to quit");
        if (auto r = create_textures(); !r) {
            GN_ERROR("Texture setup failed: {}", r.error().what());
            return false;
        }

        std::mt19937                          rng(1234);
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        auto                                  range = [&](float lo, float hi) { return lo + (hi - lo) * unit(rng); };
        for (int i = 0; i < 400; ++i) {
            const float s = range(16.0f, 56.0f);
            m_movers.push_back({m_textures[i % m_textures.size()],
                                {range(0.0f, 1280.0f), range(0.0f, 720.0f)},
                                {range(-120.0f, 120.0f), range(-120.0f, 120.0f)},
                                {s, s},
                                range(0.0f, 6.28f),
                                range(-2.0f, 2.0f),
                                gn::Color{range(0.4f, 1.0f), range(0.4f, 1.0f), range(0.4f, 1.0f), range(0.5f, 1.0f)},
                                i % 3});
        }
        return true;
    }

    void on_update(gn::f32 /*dt*/) override {
        if (engine()->input().pressed(gn::platform::Key::Escape)) engine()->request_exit();
    }

    void on_render(gn::graphics::Frame& frame) override {
        const gn::vec2 extent{static_cast<float>(frame.target_extent.width),
                              static_cast<float>(frame.target_extent.height)};
        frame.clear_color = gn::Color{0.08f, 0.09f, 0.12f, 1.0f};

        // World units are pixels, origin at the top-left: put the camera centre at half the viewport.
        engine()->camera().position = extent * 0.5f;

        auto& sprites = engine()->sprites();
        for (Mover& m : m_movers) {
            m.position += m.velocity * frame.delta;
            m.angle += m.spin * frame.delta;
            if (m.position.x < 0.0f || m.position.x > extent.x) m.velocity.x = -m.velocity.x;
            if (m.position.y < 0.0f || m.position.y > extent.y) m.velocity.y = -m.velocity.y;

            gn::graphics::Sprite s;
            s.texture  = m.texture;
            s.position = m.position;
            s.size     = m.size;
            s.rotation = m.angle;
            s.pivot    = {0.5f, 0.5f};
            s.tint     = m.tint;
            s.layer    = m.layer;
            sprites.draw(s);
        }
    }

    void on_resize(gn::u32 w, gn::u32 h) override { GN_INFO("Window resized to {}x{}", w, h); }

    void on_shutdown() override { GN_INFO("DemoApp shutting down"); }

private:
    gn::Result<void> create_textures() {
        using namespace gn;
        auto& textures = engine()->textures();

        const auto checker = graphics::make_checkerboard(8, 8, 1, {235, 235, 235, 255}, {40, 40, 90, 255});
        auto       t0      = textures.create(checker.data(), 8, 8);
        if (!t0) return Err{t0.error()};

        const u8 white[4] = {255, 255, 255, 255};
        auto     t1       = textures.create(white, 1, 1);
        if (!t1) return Err{t1.error()};

        // A disc with a soft edge: exercises alpha blending.
        constexpr u32   kDisc = 32;
        std::vector<u8> disc(kDisc * kDisc * 4, 255);
        for (u32 y = 0; y < kDisc; ++y) {
            for (u32 x = 0; x < kDisc; ++x) {
                const float d = std::hypot(x + 0.5f - kDisc / 2.0f, y + 0.5f - kDisc / 2.0f) / (kDisc / 2.0f);
                disc[(y * kDisc + x) * 4 + 3] = static_cast<u8>(255.0f * std::clamp(3.0f * (1.0f - d), 0.0f, 1.0f));
            }
        }
        auto t2 = textures.create(disc.data(), kDisc, kDisc);
        if (!t2) return Err{t2.error()};

        m_textures = {t0.value(), t1.value(), t2.value()};
        if (!m_image_path.empty()) {
            auto img = textures.load(m_image_path);
            if (!img) return Err{img.error()};
            m_textures.push_back(img.value());
        }
        return Ok();
    }

    std::string                    m_image_path;
    std::vector<gn::TextureHandle> m_textures;
    std::vector<Mover>             m_movers;
};

int main(int argc, char* argv[]) {
    gn::Engine::Config cfg;
    cfg.app_name      = "Graphyne — Sprites";
    cfg.window_width  = 1280;
    cfg.window_height = 720;
    cfg.log_file      = "graphyne.log";

    gn::Engine engine(cfg);
    DemoApp    app(argc > 1 ? argv[1] : "");
    return engine.run(app);
}
