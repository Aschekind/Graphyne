/**
 * @file memory/pool.h
 * @brief Fixed-size free-list pool allocator.
 *
 * Pre-allocates `capacity` blocks each large enough to hold a T. acquire()
 * returns one in O(1); release() returns one to the pool in O(1). When
 * the pool is exhausted acquire() returns nullptr.
 *
 * Not thread-safe (use external synchronization, or sharded pools).
 */
#pragma once

#include "core/types.h"

#include <cstddef>
#include <new>
#include <utility>

namespace gn::memory {

class PoolAllocator {
public:
    PoolAllocator() = default;
    PoolAllocator(usize block_size, usize block_alignment, usize block_count);
    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&& other) noexcept;
    PoolAllocator& operator=(PoolAllocator&& other) noexcept;

    /// Acquire one block, or nullptr if the pool is full.
    void* acquire();

    /// Return a previously-acquired block to the pool.
    void release(void* ptr);

    usize block_size()  const { return m_block_size; }
    usize block_count() const { return m_block_count; }
    usize used()        const { return m_used; }
    usize available()   const { return m_block_count - m_used; }

private:
    struct FreeNode { FreeNode* next; };

    u8*        m_buffer          = nullptr;
    FreeNode*  m_free_head       = nullptr;
    usize      m_block_size      = 0;       // bytes per block (aligned)
    usize      m_block_alignment = 0;       // alignment used at allocation
    usize      m_block_count     = 0;
    usize      m_used            = 0;
};


/// Type-safe wrapper that returns/accepts T* and constructs/destructs in place.
template <class T>
class TypedPool {
public:
    explicit TypedPool(usize count)
        : m_pool(sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T),
                 alignof(T) < alignof(void*) ? alignof(void*) : alignof(T),
                 count) {}

    template <class... Args>
    T* create(Args&&... args) {
        void* p = m_pool.acquire();
        return p ? new (p) T(std::forward<Args>(args)...) : nullptr;
    }

    void destroy(T* obj) {
        if (!obj) return;
        obj->~T();
        m_pool.release(obj);
    }

    usize used()      const { return m_pool.used(); }
    usize available() const { return m_pool.available(); }
    usize capacity()  const { return m_pool.block_count(); }

private:
    PoolAllocator m_pool;
};

} // namespace gn::memory
