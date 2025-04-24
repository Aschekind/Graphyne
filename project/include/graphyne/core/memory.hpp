/**
 * @file memory.h
 * @brief Custom memory management for Graphyne
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace graphyne::core
{

/**
 * @enum AllocationType
 * @brief Types of memory allocations
 */
enum class AllocationType
{
    General,  // General-purpose allocations
    Graphics, // Graphics-related allocations
    Audio,    // Audio-related allocations
    Physics,  // Physics-related allocations
    Script,   // Scripting-related allocations
    Temp      // Temporary allocations
};

/**
 * @class MemoryManager
 * @brief Manages memory pools and tracking for the engine
 */
class MemoryManager
{
public:
    /**
     * @brief Get singleton instance of the memory manager
     * @return Reference to the memory manager instance
     */
    static MemoryManager& getInstance();

    /**
     * @brief Initialize the memory manager
     * @param generalPoolSize Size of the general-purpose memory pool in bytes
     * @param tempPoolSize Size of the temporary memory pool in bytes
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize(size_t generalPoolSize = 64 * 1024 * 1024, size_t tempPoolSize = 32 * 1024 * 1024);

    /**
     * @brief Shutdown the memory manager and free all allocated memory
     */
    void shutdown();

    /**
     * @brief Allocate memory
     * @param size Size in bytes to allocate
     * @param alignment Memory alignment in bytes (default is 16)
     * @param type Type of allocation
     * @return Pointer to the allocated memory, or nullptr if allocation failed
     */
    void* allocate(size_t size, size_t alignment = 16, AllocationType type = AllocationType::General);

    /**
     * @brief Free allocated memory
     * @param ptr Pointer to the memory to free
     * @param type Type of allocation (must match the allocation type)
     */
    void free(void* ptr, AllocationType type = AllocationType::General);

    /**
     * @brief Get memory usage statistics
     * @param type Type of allocation to get statistics for
     * @return Total allocated bytes for the specified type
     */
    size_t getAllocatedSize(AllocationType type) const;

    /**
     * @brief Print memory usage statistics
     */
    void printStatistics() const;

private:
    // Private constructor for singleton
    MemoryManager() = default;

    // Deleted copy and move constructors and assignment operators
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    // Implementation details will be defined in the .cpp file
    struct MemoryManagerImpl;
    std::unique_ptr<MemoryManagerImpl> m_impl;
    bool m_initialized = false;
};

// Custom C++ allocator compatible with STL containers
template <typename T, AllocationType Type = AllocationType::General>
class Allocator
{
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind
    {
        using other = Allocator<U, Type>;
    };

    Allocator() noexcept = default;
    template <typename U>
    Allocator(const Allocator<U, Type>&) noexcept
    {
    }

    pointer allocate(size_type n)
    {
        void* ptr = MemoryManager::getInstance().allocate(n * sizeof(T), alignof(T), Type);
        if (!ptr)
        {
            throw std::bad_alloc();
        }
        return static_cast<pointer>(ptr);
    }

    void deallocate(pointer p, size_type) noexcept
    {
        MemoryManager::getInstance().free(p, Type);
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        new (p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }
};

template <typename T, AllocationType TypeA, typename U, AllocationType TypeB>
bool operator==(const Allocator<T, TypeA>&, const Allocator<U, TypeB>&) noexcept
{
    return TypeA == TypeB;
}

template <typename T, AllocationType TypeA, typename U, AllocationType TypeB>
bool operator!=(const Allocator<T, TypeA>&, const Allocator<U, TypeB>&) noexcept
{
    return TypeA != TypeB;
}

// Helper functions for memory allocation
template <typename T, typename... Args>
T* createObject(Args&&... args, AllocationType type = AllocationType::General)
{
    void* ptr = MemoryManager::getInstance().allocate(sizeof(T), alignof(T), type);
    if (!ptr)
    {
        return nullptr;
    }
    return new (ptr) T(std::forward<Args>(args)...);
}

template <typename T>
void destroyObject(T* object, AllocationType type = AllocationType::General)
{
    if (object)
    {
        object->~T();
        MemoryManager::getInstance().free(object, type);
    }
}

// STL container aliases using custom allocator
template <typename T, AllocationType Type = AllocationType::General>
using Vector = std::vector<T, Allocator<T, Type>>;

template <typename T, AllocationType Type = AllocationType::General>
using List = std::list<T, Allocator<T, Type>>;

template <typename Key, typename T, AllocationType Type = AllocationType::General>
using Map = std::map<Key, T, std::less<Key>, Allocator<std::pair<const Key, T>, Type>>;

template <typename Key, typename T, AllocationType Type = AllocationType::General>
using UnorderedMap =
    std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, Allocator<std::pair<const Key, T>, Type>>;

template <typename T, AllocationType Type = AllocationType::General>
using UniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

template <typename T, AllocationType Type = AllocationType::General, typename... Args>
UniquePtr<T, Type> makeUnique(Args&&... args)
{
    auto deleter = [](T* ptr) { destroyObject(ptr, Type); };
    return UniquePtr<T, Type>(createObject<T>(std::forward<Args>(args)..., Type), deleter);
}

} // namespace graphyne::core
