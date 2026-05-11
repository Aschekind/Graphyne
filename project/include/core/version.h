/**
 * @file core/version.h
 * @brief Engine version metadata.
 */
#pragma once

#include "core/types.h"

namespace gn {

struct Version {
    u32 major;
    u32 minor;
    u32 patch;
};

/// Current engine version (compile-time constant).
inline constexpr Version kEngineVersion{0, 2, 0};

/// "0.2.0"
const char* engine_version_string();

/// "Graphyne 0.2.0"
const char* engine_name();

} // namespace gn
