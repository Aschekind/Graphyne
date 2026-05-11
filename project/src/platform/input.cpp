#include "platform/input.h"

namespace gn::platform {

void Input::begin_frame() {
    m_key_prev    = m_key_down;
    m_mouse_prev  = m_mouse_down;
    m_mouse_delta = {0, 0};
    m_scroll_delta = {0.0f, 0.0f};
}

void Input::on_event(const Event& event) {
    switch (event.type) {
        case EventType::KeyDown: {
            const auto idx = static_cast<usize>(event.key.key);
            if (idx < kKeyCount) m_key_down[idx] = true;
            break;
        }
        case EventType::KeyUp: {
            const auto idx = static_cast<usize>(event.key.key);
            if (idx < kKeyCount) m_key_down[idx] = false;
            break;
        }
        case EventType::MouseDown: {
            const auto idx = static_cast<usize>(event.mouse_button.button);
            if (idx < kMouseCount) m_mouse_down[idx] = true;
            m_mouse_pos = ivec2{event.mouse_button.x, event.mouse_button.y};
            break;
        }
        case EventType::MouseUp: {
            const auto idx = static_cast<usize>(event.mouse_button.button);
            if (idx < kMouseCount) m_mouse_down[idx] = false;
            m_mouse_pos = ivec2{event.mouse_button.x, event.mouse_button.y};
            break;
        }
        case EventType::MouseMove:
            m_mouse_pos    = ivec2{event.mouse_move.x, event.mouse_move.y};
            m_mouse_delta += ivec2{event.mouse_move.dx, event.mouse_move.dy};
            break;
        case EventType::MouseScroll:
            m_scroll_delta += vec2{event.scroll.dx, event.scroll.dy};
            break;
        default: break;
    }
}

bool Input::down(Key k) const {
    const auto idx = static_cast<usize>(k);
    return idx < kKeyCount && m_key_down[idx];
}

bool Input::pressed(Key k) const {
    const auto idx = static_cast<usize>(k);
    return idx < kKeyCount && m_key_down[idx] && !m_key_prev[idx];
}

bool Input::released(Key k) const {
    const auto idx = static_cast<usize>(k);
    return idx < kKeyCount && !m_key_down[idx] && m_key_prev[idx];
}

bool Input::mouse_down(MouseButton b) const {
    const auto idx = static_cast<usize>(b);
    return idx < kMouseCount && m_mouse_down[idx];
}

bool Input::mouse_pressed(MouseButton b) const {
    const auto idx = static_cast<usize>(b);
    return idx < kMouseCount && m_mouse_down[idx] && !m_mouse_prev[idx];
}

bool Input::mouse_released(MouseButton b) const {
    const auto idx = static_cast<usize>(b);
    return idx < kMouseCount && !m_mouse_down[idx] && m_mouse_prev[idx];
}

} // namespace gn::platform
