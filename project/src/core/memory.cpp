#include "graphyne/core/memory.hpp"
#include "graphyne/utils/logger.hpp"
#include <algorithm>
#include <cstring>

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
    };

    MemoryPool generalPool;
    MemoryPool tempPool;
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

    // Initialize general pool
    m_impl->generalPool.data = new uint8_t[generalPoolSize];
    m_impl->generalPool.size = generalPoolSize;
    m_impl->generalPool.used = 0;
    m_impl->generalPool.peak = 0;

    // Initialize temporary pool
    m_impl->tempPool.data = new uint8_t[tempPoolSize];
    m_impl->tempPool.size = tempPoolSize;
    m_impl->tempPool.used = 0;
    m_impl->tempPool.peak = 0;

    m_impl->initialized = true;
    m_initialized = true;

    GN_INFO("Memory manager initialized with general pool: ",
            std::to_string(generalPoolSize),
            " bytes, temp pool: ",
            std::to_string(tempPoolSize),
            " bytes");
    return true;
}

void MemoryManager::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    printStatistics();

    delete[] m_impl->generalPool.data;
    delete[] m_impl->tempPool.data;

    m_impl.reset();
    m_initialized = false;
}

void* MemoryManager::allocate(size_t size, size_t alignment, AllocationType type)
{
    if (!m_initialized)
    {
        GN_ERROR("Memory manager not initialized");
        return nullptr;
    }

    MemoryManagerImpl::MemoryPool& pool = (type == AllocationType::Temp) ? m_impl->tempPool : m_impl->generalPool;

    // Calculate aligned size
    size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

    if (pool.used + alignedSize > pool.size)
    {
        GN_ERROR("Memory pool out of memory");
        return nullptr;
    }

    // Find aligned address
    uint8_t* ptr = pool.data + pool.used;
    size_t offset = (reinterpret_cast<uintptr_t>(ptr) + alignment - 1) & ~(alignment - 1);
    offset -= reinterpret_cast<uintptr_t>(ptr);

    pool.used += alignedSize + offset;
    pool.peak = std::max(pool.peak, pool.used);

    return ptr + offset;
}

void MemoryManager::free(void* ptr, AllocationType type)
{
    if (!m_initialized || !ptr)
    {
        return;
    }

    // In this simple implementation, we don't actually free memory
    // The pools are cleared when the memory manager is shut down
}

size_t MemoryManager::getAllocatedSize(AllocationType type) const
{
    if (!m_initialized)
    {
        return 0;
    }

    const MemoryManagerImpl::MemoryPool& pool = (type == AllocationType::Temp) ? m_impl->tempPool : m_impl->generalPool;
    return pool.used;
}

void MemoryManager::printStatistics() const
{
    if (!m_initialized)
    {
        return;
    }

    GN_INFO("Memory Statistics:");
    GN_INFO("General Pool:");
    GN_INFO("  Used: {} bytes", m_impl->generalPool.used);
    GN_INFO("  Peak: {} bytes", m_impl->generalPool.peak);
    GN_INFO("  Total: {} bytes", m_impl->generalPool.size);
    GN_INFO("Temporary Pool:");
    GN_INFO("  Used: {} bytes", m_impl->tempPool.used);
    GN_INFO("  Peak: {} bytes", m_impl->tempPool.peak);
    GN_INFO("  Total: {} bytes", m_impl->tempPool.size);
}

} // namespace graphyne::core
