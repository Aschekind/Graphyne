/**
 * @file core/handle.h
 * @brief Type-safe, generation-checked resource handles.
 *
 * Handles are 32-bit values composed of a 20-bit slot index and a 12-bit
 * generation counter. The generation lets us detect stale handles that
 * outlive their resource (use-after-free at the API layer).
 *
 * Each resource kind defines its own tag type so that handles cannot be
 * accidentally crossed (a TextureHandle cannot be passed where a
 * BufferHandle is expected).
 */
#pragma once

#include "core/types.h"

#include <compare>
#include <functional>

namespace gn {

template <class Tag>
struct Handle {
    static constexpr u32 kIndexBits = 20;
    static constexpr u32 kGenBits   = 12;
    static constexpr u32 kIndexMask = (1u << kIndexBits) - 1u;
    static constexpr u32 kGenMask   = (1u << kGenBits)   - 1u;
    static constexpr u32 kInvalid   = 0;

    u32 packed = kInvalid;

    constexpr Handle() = default;
    constexpr Handle(u32 index, u32 generation)
        : packed(((generation & kGenMask) << kIndexBits) | (index & kIndexMask))
    {}

    constexpr u32 index()      const { return packed & kIndexMask; }
    constexpr u32 generation() const { return (packed >> kIndexBits) & kGenMask; }
    constexpr bool valid()     const { return packed != kInvalid; }

    constexpr explicit operator bool() const { return valid(); }
    constexpr auto operator<=>(const Handle&) const = default;

    static constexpr Handle invalid() { return Handle{}; }
};

// Forward-declared tag types so concrete handles can be aliased without
// pulling in the resource headers.
struct TextureTag;
struct BufferTag;
struct ImageTag;
struct SamplerTag;
struct PipelineTag;
struct ShaderTag;
struct SpriteTag;
struct MaterialTag;

using TextureHandle  = Handle<TextureTag>;
using BufferHandle   = Handle<BufferTag>;
using ImageHandle    = Handle<ImageTag>;
using SamplerHandle  = Handle<SamplerTag>;
using PipelineHandle = Handle<PipelineTag>;
using ShaderHandle   = Handle<ShaderTag>;
using SpriteHandle   = Handle<SpriteTag>;
using MaterialHandle = Handle<MaterialTag>;

} // namespace gn

// std::hash specialization so handles can live in unordered containers.
namespace std {
template <class Tag>
struct hash<::gn::Handle<Tag>> {
    size_t operator()(const ::gn::Handle<Tag>& h) const noexcept {
        return std::hash<::gn::u32>{}(h.packed);
    }
};
} // namespace std
