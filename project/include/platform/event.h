/**
 * @file platform/event.h
 * @brief Engine-side event type emitted by the platform layer.
 *
 * A single discriminated struct rather than std::variant to keep
 * compile times down and the public API simple.
 */
#pragma once

#include "core/types.h"
#include "platform/key_codes.h"

namespace gn::platform {

enum class EventType : u8 {
    None,
    WindowClose,
    WindowResize,
    WindowFocusGained,
    WindowFocusLost,
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseScroll,
    TextInput,
};

struct Event {
    EventType type = EventType::None;

    // Payloads (only one is meaningful per `type`; the union members are not
    // used because we want trivial default construction and easy logging).
    struct {
        u32 width  = 0;
        u32 height = 0;
    } resize;

    struct {
        Key key      = Key::Unknown;
        u16 mods     = 0;   // bitmask of modifier keys
        bool repeat  = false;
    } key;

    struct {
        MouseButton button = MouseButton::Left;
        i32 x = 0;
        i32 y = 0;
    } mouse_button;

    struct {
        i32 x      = 0;
        i32 y      = 0;
        i32 dx     = 0;
        i32 dy     = 0;
    } mouse_move;

    struct {
        f32 dx = 0;
        f32 dy = 0;
    } scroll;

    // Up to 32 bytes of UTF-8 text input.
    char text[32] = {};
};

} // namespace gn::platform
