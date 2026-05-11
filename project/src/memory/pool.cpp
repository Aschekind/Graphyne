#include "memory/pool.h"

#include <cstdint>
#include <cstdlib>

namespace gn::memory {

namespace {
constexpr usize align_up(usize value, usize alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}
} // namespace

PoolAllocator::PoolAllocator(usize block_size, usize block_alignment, usize block_count) {
    if (block_size == 0 || block_count == 0 || block_alignment == 0) {
        return;
    }
    // Each block must be at least pointer-sized so we can thread a free-list
    // pointer through unused blocks.
    if (block_size < sizeof(FreeNode*)) block_size = sizeof(FreeNode*);
    if (block_alignment < alignof(FreeNode*)) block_alignment = alignof(FreeNode*);

    m_block_size      = align_up(block_size, block_alignment);
    m_block_alignment = block_alignment;
    m_block_count     = block_count;
    m_used            = 0;

    // Allocate aligned backing storage.
    const usize total = m_block_size * m_block_count;
    void* raw = ::operator new(total, std::align_val_t{block_alignment});
    m_buffer = static_cast<u8*>(raw);

    // Build the initial free list (singly linked).
    FreeNode* prev = nullptr;
    for (usize i = m_block_count; i-- > 0;) {
        auto* node  = reinterpret_cast<FreeNode*>(m_buffer + i * m_block_size);
        node->next  = prev;
        prev        = node;
    }
    m_free_head = prev;
}

PoolAllocator::~PoolAllocator() {
    if (m_buffer) {
        ::operator delete(m_buffer, std::align_val_t{m_block_alignment});
    }
}

PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
    : m_buffer(other.m_buffer),
      m_free_head(other.m_free_head),
      m_block_size(other.m_block_size),
      m_block_alignment(other.m_block_alignment),
      m_block_count(other.m_block_count),
      m_used(other.m_used) {
    other.m_buffer          = nullptr;
    other.m_free_head       = nullptr;
    other.m_block_size      = 0;
    other.m_block_alignment = 0;
    other.m_block_count     = 0;
    other.m_used            = 0;
}

PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
    if (this != &other) {
        if (m_buffer) {
            ::operator delete(m_buffer, std::align_val_t{m_block_alignment});
        }
        m_buffer          = other.m_buffer;
        m_free_head       = other.m_free_head;
        m_block_size      = other.m_block_size;
        m_block_alignment = other.m_block_alignment;
        m_block_count     = other.m_block_count;
        m_used            = other.m_used;
        other.m_buffer          = nullptr;
        other.m_free_head       = nullptr;
        other.m_block_size      = 0;
        other.m_block_alignment = 0;
        other.m_block_count     = 0;
        other.m_used            = 0;
    }
    return *this;
}

void* PoolAllocator::acquire() {
    if (!m_free_head) return nullptr;
    FreeNode* node = m_free_head;
    m_free_head    = node->next;
    ++m_used;
    return node;
}

void PoolAllocator::release(void* ptr) {
    if (!ptr) return;
    auto* node  = static_cast<FreeNode*>(ptr);
    node->next  = m_free_head;
    m_free_head = node;
    --m_used;
}

} // namespace gn::memory
