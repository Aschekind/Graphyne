/**
 * @file core/slot_map.h
 * @brief A SlotMap container — dense storage of T addressed by sparse Handles.
 *
 * Provides O(1) insert, erase, and lookup with generation-checked handles
 * that detect stale references. Used as the backing store for resource
 * managers (textures, buffers, sprites, …).
 *
 * Internally:
 *   - `m_slots[i]` is the indirection record (data index + generation).
 *   - `m_data`    is the dense, contiguous array of T (cache-friendly).
 *   - When a slot is erased, its index is pushed onto `m_free_list`
 *     and its generation is bumped — old handles to that slot become
 *     invalid on the next lookup.
 *
 * Thread safety: not internally synchronized. The owning manager should
 * guard access if used from multiple threads.
 */
#pragma once

#include "core/handle.h"
#include "core/types.h"

#include <cassert>
#include <utility>
#include <vector>

namespace gn {

template <class T, class Tag>
class SlotMap {
public:
    using HandleT = Handle<Tag>;

    SlotMap() {
        // Reserve slot 0 — kept permanently invalid so a default-constructed
        // Handle (packed == 0) never points anywhere.
        m_slots.push_back(Slot{kInvalid, 0});
    }

    template <class... Args>
    HandleT emplace(Args&&... args) {
        u32 slot_index;
        if (!m_free_list.empty()) {
            slot_index = m_free_list.back();
            m_free_list.pop_back();
        } else {
            slot_index = static_cast<u32>(m_slots.size());
            m_slots.push_back(Slot{kInvalid, 0});
        }

        Slot& slot = m_slots[slot_index];
        slot.data_index = static_cast<u32>(m_data.size());
        m_data.emplace_back(std::forward<Args>(args)...);
        m_data_to_slot.push_back(slot_index);

        return HandleT{slot_index, slot.generation};
    }

    HandleT insert(const T& value) { return emplace(value); }
    HandleT insert(T&& value)      { return emplace(std::move(value)); }

    bool contains(HandleT h) const {
        if (!h.valid()) return false;
        const u32 idx = h.index();
        if (idx >= m_slots.size()) return false;
        const Slot& slot = m_slots[idx];
        return slot.data_index != kInvalid && slot.generation == h.generation();
    }

    T* get(HandleT h) {
        if (!contains(h)) return nullptr;
        return &m_data[m_slots[h.index()].data_index];
    }

    const T* get(HandleT h) const {
        if (!contains(h)) return nullptr;
        return &m_data[m_slots[h.index()].data_index];
    }

    bool erase(HandleT h) {
        if (!contains(h)) return false;

        const u32 slot_index = h.index();
        Slot& slot = m_slots[slot_index];
        const u32 data_idx = slot.data_index;
        const u32 last_idx = static_cast<u32>(m_data.size()) - 1;

        // Swap-and-pop the dense data array.
        if (data_idx != last_idx) {
            m_data[data_idx] = std::move(m_data[last_idx]);
            const u32 moved_slot = m_data_to_slot[last_idx];
            m_data_to_slot[data_idx] = moved_slot;
            m_slots[moved_slot].data_index = data_idx;
        }
        m_data.pop_back();
        m_data_to_slot.pop_back();

        slot.data_index = kInvalid;
        // Bump the generation (wrap is fine; collision probability is 1/4096).
        slot.generation = (slot.generation + 1) & HandleT::kGenMask;
        // Don't recycle the all-zero (slot=0,gen=0) state.
        if (slot_index != 0) {
            m_free_list.push_back(slot_index);
        }
        return true;
    }

    void clear() {
        m_slots.clear();
        m_data.clear();
        m_data_to_slot.clear();
        m_free_list.clear();
        m_slots.push_back(Slot{kInvalid, 0});
    }

    usize size()      const { return m_data.size(); }
    bool  empty()     const { return m_data.empty(); }

    // Dense iteration over the values.
    auto begin()       { return m_data.begin(); }
    auto end()         { return m_data.end(); }
    auto begin() const { return m_data.begin(); }
    auto end()   const { return m_data.end(); }

private:
    struct Slot {
        u32 data_index;   // kInvalid when slot is free
        u32 generation;   // 0..kGenMask
    };
    static constexpr u32 kInvalid = ~0u;

    std::vector<Slot> m_slots;          // indirection
    std::vector<T>    m_data;           // dense storage
    std::vector<u32>  m_data_to_slot;   // m_data[i] -> owning slot index
    std::vector<u32>  m_free_list;
};

} // namespace gn
