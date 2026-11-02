#include "graphics/image_loader.h"

#include <fstream>
#include <iterator>
#include <limits>

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push, 0)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include <stb_image.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace gn::graphics {

Result<ImageData> load_image_from_memory(std::span<const u8> bytes) {
    if (bytes.empty() || bytes.size() > static_cast<usize>(std::numeric_limits<int>::max())) {
        return Err{Error{"load_image: empty or oversized input"}};
    }
    int      w        = 0;
    int      h        = 0;
    int      channels = 0;
    stbi_uc* pixels   = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &channels, 4);
    if (!pixels) return Err{Error{std::string("load_image: ") + stbi_failure_reason()}};

    ImageData image;
    image.width  = static_cast<u32>(w);
    image.height = static_cast<u32>(h);
    image.rgba.assign(pixels, pixels + static_cast<usize>(w) * static_cast<usize>(h) * 4);
    stbi_image_free(pixels);
    return image;
}

Result<ImageData> load_image(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return Err{Error{"load_image: cannot open " + path}};
    const std::vector<u8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto                  image = load_image_from_memory(bytes);
    if (!image) return Err{Error{image.error().what() + " (" + path + ")"}};
    return image;
}

} // namespace gn::graphics
