/**
 * @file resources/asset_paths.h
 * @brief Asset path resolution.
 *
 * GRAPHYNE_DEFAULT_ASSETS_DIR is provided by CMake (configure-time). It
 * points at the source-tree's examples/assets directory in development
 * builds. Override at runtime by passing an explicit absolute or relative
 * path to ResourceManager::load_texture.
 */
#pragma once

#include <string>
#include <string_view>

namespace gn::resources {

/// Compile-time default — points at the source tree's `examples/assets`.
const char* default_assets_dir();

/// Resolve a path relative to `default_assets_dir()` if it is not already
/// absolute. Otherwise return it unchanged.
std::string resolve_asset(std::string_view path);

} // namespace gn::resources
