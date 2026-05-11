/**
 * @file platform/key_codes.h
 * @brief Platform-agnostic key and mouse button identifiers.
 *
 * Mapped from SDL scancodes internally; not the same set as SDL_Keycode.
 */
#pragma once

#include "core/types.h"

namespace gn::platform {

enum class Key : u16 {
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Digits
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Whitespace / control
    Space, Tab, Enter, Escape, Backspace, Delete,

    // Arrows
    Left, Right, Up, Down,

    // Modifiers
    LeftShift, RightShift,
    LeftCtrl,  RightCtrl,
    LeftAlt,   RightAlt,

    Count
};

enum class MouseButton : u8 {
    Left   = 0,
    Right  = 1,
    Middle = 2,
    X1     = 3,
    X2     = 4,
    Count
};

} // namespace gn::platform
