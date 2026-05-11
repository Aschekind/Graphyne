/**
 * @file core/types.h
 * @brief Common scalar and vector type aliases used across the engine.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace gn {

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using isize = std::ptrdiff_t;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;
using uvec2 = glm::uvec2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using quat = glm::quat;

// A platform-agnostic size representation in pixels.
struct Extent2D {
    u32 width  = 0;
    u32 height = 0;

    constexpr bool operator==(const Extent2D&) const = default;
};

} // namespace gn
