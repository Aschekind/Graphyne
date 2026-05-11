#include "resources/image_loader.h"

#include "utils/logger.h"

// This TU is the single home of the stb_image implementation.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include <stb_image.h>

#include <cstring>

namespace gn::resources {

Result<ImagePixels> load_image_rgba8(const std::string& path) {
    int w = 0, h = 0, channels = 0;
    stbi_uc* raw = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!raw) {
        return Err{Error{"stbi_load failed for '" + path + "': " + (stbi_failure_reason() ? stbi_failure_reason() : "unknown")}};
    }
    if (w <= 0 || h <= 0) {
        stbi_image_free(raw);
        return Err{Error{"Decoded image has zero dimension: " + path}};
    }

    ImagePixels img;
    img.width    = static_cast<u32>(w);
    img.height   = static_cast<u32>(h);
    img.channels = 4;
    const usize byte_count = static_cast<usize>(w) * static_cast<usize>(h) * 4u;
    img.data.resize(byte_count);
    std::memcpy(img.data.data(), raw, byte_count);
    stbi_image_free(raw);

    GN_INFO("Decoded image: {} ({}x{}, {} channels source, RGBA8 out)", path, w, h, channels);
    return img;
}

} // namespace gn::resources
