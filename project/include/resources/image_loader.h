/**
 * @file resources/image_loader.h
 * @brief Decode PNG / JPG / BMP / TGA / etc. to an RGBA8 pixel buffer.
 *
 * Backed by stb_image. The loader always forces 4 channels (RGBA8) for a
 * predictable upload path; PNG alpha is preserved, opaque images get a
 * fully-opaque alpha channel.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <string>
#include <vector>

namespace gn::resources {

struct ImagePixels {
    u32             width    = 0;
    u32             height   = 0;
    u32             channels = 4;       // always 4 (RGBA8) post-decode
    std::vector<u8> data;                // size = width * height * 4
};

/// Load an image from disk and decode it to tightly-packed RGBA8 pixels.
Result<ImagePixels> load_image_rgba8(const std::string& absolute_path);

} // namespace gn::resources
