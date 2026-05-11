/**
 * @file platform/input.h
 * @brief Polled keyboard / mouse state with edge detection.
 *
 * The Input class consumes events fed by Window::pump_events and exposes
 * a per-frame view: down() returns "currently held", pressed() returns
 * "just transitioned to down this frame", released() returns "just
 * transitioned to up this frame".
 *
 * begin_frame() must be called once per frame before pump_events().
 */
#pragma once

#include "core/types.h"
#include "platform/event.h"
#include "platform/key_codes.h"

#include <array>

namespace gn::platform {

class Input {
public:
    void begin_frame();

    /// Feed an event from Window::pump_events into the input state.
    void on_event(const Event& event);

    bool down(Key k)     const;
    bool pressed(Key k)  const;   // edge: not-down -> down this frame
    bool released(Key k) const;   // edge: down -> not-down this frame

    bool mouse_down(MouseButton b)     const;
    bool mouse_pressed(MouseButton b)  const;
    bool mouse_released(MouseButton b) const;

    /// Current mouse position in window coordinates.
    ivec2 mouse_position() const { return m_mouse_pos; }
    /// Movement since the previous begin_frame().
    ivec2 mouse_delta()    const { return m_mouse_delta; }
    /// Scroll wheel delta since the previous begin_frame().
    vec2  scroll_delta()   const { return m_scroll_delta; }

private:
    static constexpr usize kKeyCount   = static_cast<usize>(Key::Count);
    static constexpr usize kMouseCount = static_cast<usize>(MouseButton::Count);

    std::array<bool, kKeyCount>   m_key_down    = {};
    std::array<bool, kKeyCount>   m_key_prev    = {};
    std::array<bool, kMouseCount> m_mouse_down  = {};
    std::array<bool, kMouseCount> m_mouse_prev  = {};

    ivec2 m_mouse_pos   {0, 0};
    ivec2 m_mouse_delta {0, 0};
    vec2  m_scroll_delta{0, 0};
};

} // namespace gn::platform
