/**
 * @file graphics/color.h
 * @brief Color utilities.
 */
#pragma once

#include "core/types.h"

namespace gn::graphics {

struct Color {
    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    f32 a = 1.0f;

    constexpr Color() = default;
    constexpr Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color from_u8(u8 r, u8 g, u8 b, u8 a = 255) {
        return Color{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

    constexpr vec4 as_vec4() const { return vec4{r, g, b, a}; }
};

namespace colors {
inline constexpr Color Black   {0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr Color White   {1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr Color Red     {1.0f, 0.0f, 0.0f, 1.0f};
inline constexpr Color Green   {0.0f, 1.0f, 0.0f, 1.0f};
inline constexpr Color Blue    {0.0f, 0.0f, 1.0f, 1.0f};
inline constexpr Color CornflowerBlue{0.39f, 0.58f, 0.93f, 1.0f};
} // namespace colors

} // namespace gn::graphics

namespace gn { using graphics::Color; }
