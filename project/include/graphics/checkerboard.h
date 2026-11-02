/**
 * @file graphics/checkerboard.h
 * @brief Procedural test texture.
 */
#pragma once

#include "core/types.h"

#include <array>
#include <vector>

namespace gn::graphics {

using Rgba8 = std::array<u8, 4>;

/// width*height RGBA8 pixels, alternating `a` and `b` in cells of `cell` pixels; the top-left cell is `a`.
std::vector<u8> make_checkerboard(u32 width, u32 height, u32 cell, Rgba8 a, Rgba8 b);

} // namespace gn::graphics
