#include "graphyne/core/memory.hpp"
#include "graphyne/utils/logger.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace graphyne::core
{

struct MemoryManager::MemoryManagerImpl
{
    struct MemoryPool
    {
        uint8_t* data = nullptr;
        size_t size = 0;
        size_t used = 0;
        size_t peak = 0;
        mutable std::mutex mutex;

        // Track allocations with address → size mapping
        std::unordered_map<void*, size_t> allocations;
    };

    // One pool for each allocation type
    std::unordered_map<AllocationType, MemoryPool> pools;
    bool initialized = false;
};

MemoryManager& MemoryManager::getInstance()
{
    static MemoryManager instance;
    return instance;
}

bool MemoryManager::initialize(size_t generalPoolSize, size_t tempPoolSize)
{
    if (m_initialized)
    {
        GN_WARNING("Memory manager already initialized");
        return true;
    }

    m_impl = std::make_unique<MemoryManagerImpl>();

    // Initialize all pools with appropriate sizes
    const size_t defaultPoolSize = 16 * 1024 * 1024; // 16MB default for specialized pools

    // General pool
    m_impl->pools[AllocationType::General].data = new uint8_t[generalPoolSize];
    m_impl->pools[AllocationType::General].size = generalPoolSize;

    // Temp pool
    m_impl->pools[AllocationType::Temp].data = new uint8_t[tempPoolSize];
    m_impl->pools[AllocationType::Temp].size = tempPoolSize;

    // Specialized pools
    m_impl->pools[AllocationType::Graphics].data = new uint8_t[defaultPoolSize];
    m_impl->pools[AllocationType::Graphics].size = defaultPoolSize;

    m_impl->pools[AllocationType::Audio].data = new uint8_t[defaultPoolSize];
    m_impl->pools[AllocationType::Audio].size = defaultPoolSize;

    m_impl->pools[AllocationType::Physics].data = new uint8_t[defaultPoolSize];
    m_impl->pools[AllocationType::Physics].size = defaultPoolSize;

    m_impl->pools[AllocationType::Script].data = new uint8_t[defaultPoolSize];
    m_impl->pools[AllocationType::Script].size = defaultPoolSize;

    m_impl->initialized = true;
    m_initialized = true;

    GN_INFO("Memory manager initialized with pools:");
    GN_INFO("  General: {} bytes", generalPoolSize);
    GN_INFO("  Temp: {} bytes", tempPoolSize);
    GN_INFO("  Graphics: {} bytes", defaultPoolSize);
    GN_INFO("  Audio: {} bytes", defaultPoolSize);
    GN_INFO("  Physics: {} bytes", defaultPoolSize);
    GN_INFO("  Script: {} bytes", defaultPoolSize);

    return true;
}

void MemoryManager::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    printStatistics();

    // Clean up all pools
    for (auto& [type, pool] : m_impl->pools)
    {
        delete[] pool.data;
        pool.data = nullptr;
    }

    m_impl.reset();
    m_initialized = false;

    GN_INFO("Memory manager shutdown complete");
}

void* MemoryManager::allocate(size_t size, size_t alignment, AllocationType type)
{
    if (!m_initialized)
    {
        GN_ERROR("Memory manager not initialized");
        return nullptr;
    }

    // Check if the pool exists for this allocation type
    if (m_impl->pools.find(type) == m_impl->pools.end())
    {
        GN_ERROR("Unknown allocation type");
        return nullptr;
    }

    MemoryManagerImpl::MemoryPool& pool = m_impl->pools[type];

    // Lock the mutex for thread safety
    std::lock_guard<std::mutex> lock(pool.mutex);

    // Calculate aligned size
    size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

    // Store allocation header (metadata for free operation)
    constexpr size_t headerSize = sizeof(size_t);
    size_t totalSize = alignedSize + headerSize;

    if (pool.used + totalSize > pool.size)
    {
        GN_ERROR("Memory pool out of memory. Type: {}, Requested: {} bytes, Available: {} bytes",
                static_cast<int>(type), totalSize, pool.size - pool.used);
        return nullptr;
    }

    // Find aligned address (after header)
    uint8_t* basePtr = pool.data + pool.used;
    uint8_t* dataPtr = basePtr + headerSize;

    // Align the data pointer
    size_t offset = (reinterpret_cast<uintptr_t>(dataPtr) + alignment - 1) & ~(alignment - 1);
    offset -= reinterpret_cast<uintptr_t>(dataPtr);

    // Adjust the base pointer to account for alignment
    basePtr += offset;
    dataPtr = basePtr + headerSize;

    // Store the allocation size in the header
    *reinterpret_cast<size_t*>(basePtr) = alignedSize;

    // Update pool usage
    pool.used += totalSize + offset;
    pool.peak = std::max(pool.peak, pool.used);

    // Track this allocation
    pool.allocations[dataPtr] = alignedSize;

    return dataPtr;
}

void MemoryManager::free(void* ptr, AllocationType type)
{
    if (!m_initialized || !ptr)
    {
        return;
    }

    // Check if the pool exists for this allocation type
    if (m_impl->pools.find(type) == m_impl->pools.end())
    {
        GN_ERROR("Unknown allocation type in free operation");
        return;
    }

    MemoryManagerImpl::MemoryPool& pool = m_impl->pools[type];

    // Lock the mutex for thread safety
    std::lock_guard<std::mutex> lock(pool.mutex);

    // Find the allocation
    auto it = pool.allocations.find(ptr);
    if (it == pool.allocations.end())
    {
        GN_ERROR("Attempting to free untracked memory at address {}", ptr);
        return;
    }

    // Remove from tracking
    pool.allocations.erase(it);

    // Note: In this simple pool allocator, we don't actually free memory
    // Individual allocations inside a pool are only tracked for statistics
    // The entire pool will be freed when the memory manager is shut down
}

size_t MemoryManager::getAllocatedSize(AllocationType type) const
{
    if (!m_initialized)
    {
        return 0;
    }

    auto it = m_impl->pools.find(type);
    if (it == m_impl->pools.end())
    {
        return 0;
    }

    // Lock for thread safety
    std::lock_guard<std::mutex> lock(it->second.mutex);
    return it->second.used;
}

void MemoryManager::printStatistics() const
{
    if (!m_initialized)
    {
        GN_WARNING("Memory manager not initialized, no statistics available");
        return;
    }

    GN_INFO("Memory Statistics:");

    for (const auto& [type, pool] : m_impl->pools)
    {
        const char* typeName = "Unknown";
        switch (type)
        {
            case AllocationType::General: typeName = "General"; break;
            case AllocationType::Graphics: typeName = "Graphics"; break;
            case AllocationType::Audio: typeName = "Audio"; break;
            case AllocationType::Physics: typeName = "Physics"; break;
            case AllocationType::Script: typeName = "Script"; break;
            case AllocationType::Temp: typeName = "Temporary"; break;
        }

        std::lock_guard<std::mutex> lock(pool.mutex);

        GN_INFO("{} Pool:", typeName);
        GN_INFO("  Used: {} bytes ({:.2f}%)",
                pool.used,
                (pool.size > 0) ? (static_cast<float>(pool.used) / pool.size * 100.0f) : 0.0f);
        GN_INFO("  Peak: {} bytes ({:.2f}%)",
                pool.peak,
                (pool.size > 0) ? (static_cast<float>(pool.peak) / pool.size * 100.0f) : 0.0f);
        GN_INFO("  Total: {} bytes", pool.size);
        GN_INFO("  Active allocations: {}", pool.allocations.size());
    }
}

} // namespace graphyne::core
