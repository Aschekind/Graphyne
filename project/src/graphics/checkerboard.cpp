#include "graphics/checkerboard.h"

namespace gn::graphics {

std::vector<u8> make_checkerboard(u32 width, u32 height, u32 cell, Rgba8 a, Rgba8 b) {
    if (cell == 0) cell = 1;
    std::vector<u8> pixels;
    pixels.reserve(static_cast<usize>(width) * height * 4);
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            const Rgba8& c = ((x / cell + y / cell) % 2 == 0) ? a : b;
            pixels.insert(pixels.end(), c.begin(), c.end());
        }
    }
    return pixels;
}

} // namespace gn::graphics
