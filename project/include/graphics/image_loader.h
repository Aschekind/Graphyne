/**
 * @file graphics/image_loader.h
 * @brief Decode image files (PNG, JPEG, ...) to RGBA8 with stb_image (ADR 0004).
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <span>
#include <string>
#include <vector>

namespace gn::graphics {

struct ImageData {
    u32             width  = 0;
    u32             height = 0;
    std::vector<u8> rgba; ///< width * height * 4 bytes, row 0 at the top
};

/// Always 4 channels: missing alpha becomes 255.
Result<ImageData> load_image(const std::string& path);
Result<ImageData> load_image_from_memory(std::span<const u8> bytes);

} // namespace gn::graphics
