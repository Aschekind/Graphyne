#include "resources/asset_paths.h"

#ifndef GRAPHYNE_ASSETS_DIR
#define GRAPHYNE_ASSETS_DIR ""
#endif

namespace gn::resources {

const char* default_assets_dir() {
    return GRAPHYNE_ASSETS_DIR;
}

static bool is_absolute(std::string_view p) {
    if (p.empty()) return false;
    if (p[0] == '/' || p[0] == '\\') return true;
    if (p.size() >= 2 && p[1] == ':') return true;   // Windows drive
    return false;
}

std::string resolve_asset(std::string_view path) {
    if (is_absolute(path)) return std::string(path);
    std::string base = default_assets_dir();
    if (base.empty()) return std::string(path);
    if (base.back() != '/' && base.back() != '\\') base.push_back('/');
    return base + std::string(path);
}

} // namespace gn::resources
