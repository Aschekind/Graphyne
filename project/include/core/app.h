/**
 * @file core/app.h
 * @brief Base class for user applications running on top of the engine.
 *
 * Override the lifecycle hooks to plug your game logic into the engine
 * without touching the engine internals:
 *
 *   class MyGame : public gn::App {
 *       void on_start()             override { ... }
 *       void on_update(gn::f32 dt)  override { ... }
 *       void on_render(gn::Frame&)  override { ... }
 *       void on_shutdown()          override { ... }
 *   };
 */
#pragma once

#include "core/types.h"

namespace gn {

class Engine;
namespace graphics { class Frame; }
namespace platform { struct Event; }

class App {
public:
    App() = default;
    virtual ~App() = default;

    App(const App&)            = delete;
    App& operator=(const App&) = delete;
    App(App&&)                 = delete;
    App& operator=(App&&)      = delete;

    /// Called once after the engine has finished initializing. Return false
    /// to abort startup.
    virtual bool on_start() { return true; }

    /// Called once per frame before rendering. `dt` is the time, in seconds,
    /// elapsed since the previous on_update call.
    virtual void on_update(f32 dt) { (void)dt; }

    /// Called once per frame after on_update. Use the supplied Frame to
    /// submit draw calls.
    virtual void on_render(graphics::Frame& frame) { (void)frame; }

    /// Called whenever the swap-chain target is resized.
    virtual void on_resize(u32 width, u32 height) { (void)width; (void)height; }

    /// Called for every platform / input event after polling.
    virtual void on_event(const platform::Event& event) { (void)event; }

    /// Called once before the engine tears down its subsystems.
    virtual void on_shutdown() {}

    /// Set during Engine::run() so user code can request shutdown
    /// (`engine().request_exit()`).
    Engine* engine() const { return m_engine; }

private:
    friend class Engine;
    Engine* m_engine = nullptr;
};

} // namespace gn
