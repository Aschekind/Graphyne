/**
 * @file memory/arena.h
 * @brief Bump (arena) allocator.
 *
 * Single contiguous buffer. allocate() bumps a cursor; reset() rewinds
 * the cursor to zero. Individual frees are not supported by design;
 * intended for per-frame or per-scope transient data.
 *
 * Not thread-safe.
 */
#pragma once

#include "core/types.h"

#include <cstddef>
#include <new>
#include <utility>

namespace gn::memory {

class Arena {
public:
    Arena() = default;
    explicit Arena(usize capacity_bytes);
    ~Arena();

    Arena(const Arena&)            = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& other) noexcept;
    Arena& operator=(Arena&& other) noexcept;

    /// Allocate `bytes` aligned to `alignment` (must be power of two).
    /// Returns nullptr if the arena is exhausted.
    void* allocate(usize bytes, usize alignment = alignof(std::max_align_t));

    template <class T, class... Args>
    T* create(Args&&... args) {
        void* p = allocate(sizeof(T), alignof(T));
        return p ? new (p) T(std::forward<Args>(args)...) : nullptr;
    }

    /// Reset the cursor back to 0. Does NOT call destructors — caller must
    /// ensure no live objects remain.
    void reset();

    /// Total bytes capacity.
    usize capacity() const { return m_capacity; }

    /// Bytes currently allocated.
    usize used() const { return m_offset; }

    /// Peak bytes used since the last reset (or construction).
    usize peak() const { return m_peak; }

private:
    u8*   m_buffer   = nullptr;
    usize m_capacity = 0;
    usize m_offset   = 0;
    usize m_peak     = 0;
};

} // namespace gn::memory
