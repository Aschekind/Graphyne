/**
 * @file simple_window.cpp
 * @brief Minimal example: open a window, draw a checkerboard texture, ESC to quit.
 *
 * Demonstrates the App lifecycle and the RHI building blocks the sprite renderer will use: a texture
 * uploaded through a staging buffer, a nearest sampler, a descriptor set, shaders loaded next to the
 * executable and a dynamic-rendering pipeline. The fullscreen triangle has no vertex buffer.
 */
#include "core/app.h"
#include "core/engine.h"
#include "graphics/checkerboard.h"
#include "graphics/frame.h"
#include "graphics/renderer.h"
#include "platform/event.h"
#include "platform/input.h"
#include "platform/key_codes.h"
#include "platform/platform.h"
#include "rhi/device.h"
#include "rhi/image.h"
#include "rhi/pipeline.h"
#include "rhi/shader.h"
#include "rhi/swapchain.h"
#include "utils/logger.h"

#include <cmath>

class DemoApp final : public gn::App {
public:
    bool on_start() override {
        GN_INFO("DemoApp started — press ESC to quit");
        if (auto r = create_resources(); !r) {
            GN_ERROR("Resource setup failed: {}", r.error().what());
            return false;
        }
        engine()->renderer().set_pass_callback([this](gn::graphics::Frame& frame) { draw(frame); });
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
        // Animated clear color (visible only if the texture does not cover the screen).
        const float t     = m_time;
        frame.clear_color = gn::Color{
            0.5f + 0.5f * std::sin(t * 0.7f),
            0.5f + 0.5f * std::sin(t * 0.9f + 1.0f),
            0.5f + 0.5f * std::sin(t * 1.1f + 2.0f),
            1.0f,
        };
    }

    void on_resize(gn::u32 w, gn::u32 h) override { GN_INFO("Window resized to {}x{}", w, h); }

    void on_shutdown() override {
        // The engine has waited for the GPU to be idle. Destroy in reverse order of creation.
        engine()->renderer().set_pass_callback({});
        m_pipeline.shutdown();
        m_descriptors.shutdown();
        m_sampler.shutdown();
        m_texture.shutdown();
        m_fragment.shutdown();
        m_vertex.shutdown();
        GN_INFO("DemoApp shutting down");
    }

private:
    gn::Result<void> create_resources() {
        using namespace gn;
        rhi::Device& device = engine()->device();

        const std::string shaders = platform::base_path() + "shaders/";
        if (auto r = m_vertex.load(device, shaders + "fullscreen.vert.spv"); !r) return r;
        if (auto r = m_fragment.load(device, shaders + "texture.frag.spv"); !r) return r;

        constexpr u32 kSize  = 8;
        const auto    pixels = graphics::make_checkerboard(kSize, kSize, 1, {235, 235, 235, 255}, {40, 40, 90, 255});
        if (auto r = m_texture.initialize(device, kSize, kSize); !r) return r;
        if (auto r = m_texture.upload_rgba8(pixels.data()); !r) return r;
        if (auto r = m_sampler.initialize(device, rhi::Sampler::Filter::Nearest); !r) return r;

        if (auto r = m_descriptors.initialize(device, 1); !r) return r;
        auto set = m_descriptors.allocate();
        if (!set) return Err{set.error()};
        m_set = set.value();
        m_descriptors.update(m_set, m_texture.view(), m_sampler.handle());

        rhi::PipelineDesc desc;
        desc.vertex       = m_vertex.module();
        desc.fragment     = m_fragment.module();
        desc.color_format = engine()->swapchain().image_format();
        desc.set_layouts  = {m_descriptors.layout()};
        return m_pipeline.initialize(device, desc);
    }

    void draw(gn::graphics::Frame& frame) {
        const VkViewport viewport{0.0f,
                                  0.0f,
                                  static_cast<float>(frame.target_extent.width),
                                  static_cast<float>(frame.target_extent.height),
                                  0.0f,
                                  1.0f};
        const VkRect2D   scissor{{0, 0}, {frame.target_extent.width, frame.target_extent.height}};
        vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle());
        vkCmdSetViewport(frame.cmd, 0, 1, &viewport);
        vkCmdSetScissor(frame.cmd, 0, 1, &scissor);
        vkCmdBindDescriptorSets(
            frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(), 0, 1, &m_set, 0, nullptr);
        vkCmdDraw(frame.cmd, 3, 1, 0, 0);
    }

    float                       m_time = 0.0f;
    gn::rhi::Shader             m_vertex;
    gn::rhi::Shader             m_fragment;
    gn::rhi::Image              m_texture;
    gn::rhi::Sampler            m_sampler;
    gn::rhi::SamplerDescriptors m_descriptors;
    VkDescriptorSet             m_set = VK_NULL_HANDLE;
    gn::rhi::Pipeline           m_pipeline;
};

int main(int /*argc*/, char* /*argv*/[]) {
    gn::Engine::Config cfg;
    cfg.app_name      = "Graphyne — Simple Window";
    cfg.window_width  = 1280;
    cfg.window_height = 720;
    cfg.log_file      = "graphyne.log";

    gn::Engine engine(cfg);
    DemoApp    app;
    return engine.run(app);
}
