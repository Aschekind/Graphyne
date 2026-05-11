#include "memory/arena.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace gn::memory {

namespace {
constexpr bool is_power_of_two(usize x) {
    return x != 0 && (x & (x - 1)) == 0;
}

constexpr usize align_up(usize value, usize alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}
} // namespace

Arena::Arena(usize capacity_bytes)
    : m_buffer(capacity_bytes ? new u8[capacity_bytes] : nullptr),
      m_capacity(capacity_bytes),
      m_offset(0),
      m_peak(0) {}

Arena::~Arena() {
    delete[] m_buffer;
}

Arena::Arena(Arena&& other) noexcept
    : m_buffer(other.m_buffer),
      m_capacity(other.m_capacity),
      m_offset(other.m_offset),
      m_peak(other.m_peak) {
    other.m_buffer   = nullptr;
    other.m_capacity = 0;
    other.m_offset   = 0;
    other.m_peak     = 0;
}

Arena& Arena::operator=(Arena&& other) noexcept {
    if (this != &other) {
        delete[] m_buffer;
        m_buffer    = other.m_buffer;
        m_capacity  = other.m_capacity;
        m_offset    = other.m_offset;
        m_peak      = other.m_peak;
        other.m_buffer   = nullptr;
        other.m_capacity = 0;
        other.m_offset   = 0;
        other.m_peak     = 0;
    }
    return *this;
}

void* Arena::allocate(usize bytes, usize alignment) {
    if (!is_power_of_two(alignment) || bytes == 0 || m_buffer == nullptr) {
        return nullptr;
    }

    const auto base_address = reinterpret_cast<std::uintptr_t>(m_buffer);
    const auto current_address = base_address + m_offset;
    const auto aligned_address = align_up(current_address, alignment);
    const usize aligned_offset = static_cast<usize>(aligned_address - base_address);
    if (aligned_offset + bytes > m_capacity) {
        return nullptr;
    }

    void* ptr = m_buffer + aligned_offset;
    m_offset = aligned_offset + bytes;
    m_peak   = std::max(m_peak, m_offset);
    return ptr;
}

void Arena::reset() {
    m_offset = 0;
}

} // namespace gn::memory
