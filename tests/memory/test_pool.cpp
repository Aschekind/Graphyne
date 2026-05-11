#include <gtest/gtest.h>

#include "memory/pool.h"

#include <cstdint>
#include <set>

using gn::memory::PoolAllocator;
using gn::memory::TypedPool;

TEST(PoolAllocator, AcquiresAllBlocks) {
    PoolAllocator pool(sizeof(int), alignof(int), 4);
    void* a = pool.acquire();
    void* b = pool.acquire();
    void* c = pool.acquire();
    void* d = pool.acquire();
    EXPECT_NE(a, nullptr);
    EXPECT_NE(b, nullptr);
    EXPECT_NE(c, nullptr);
    EXPECT_NE(d, nullptr);
    EXPECT_EQ(pool.acquire(), nullptr);  // exhausted
}

TEST(PoolAllocator, ReleaseReusesBlocks) {
    PoolAllocator pool(sizeof(int), alignof(int), 2);
    void* a = pool.acquire();
    void* b = pool.acquire();
    EXPECT_EQ(pool.acquire(), nullptr);
    pool.release(a);
    void* a2 = pool.acquire();
    EXPECT_NE(a2, nullptr);
    EXPECT_EQ(a2, a);   // free-list is LIFO
    pool.release(b);
    pool.release(a2);
}

TEST(PoolAllocator, BlocksAreUnique) {
    PoolAllocator pool(sizeof(int), alignof(int), 16);
    std::set<void*> seen;
    for (int i = 0; i < 16; ++i) {
        void* p = pool.acquire();
        ASSERT_NE(p, nullptr);
        EXPECT_TRUE(seen.insert(p).second);  // never returned before
    }
}

TEST(PoolAllocator, RespectsAlignment) {
    PoolAllocator pool(8, 64, 4);
    for (int i = 0; i < 4; ++i) {
        void* p = pool.acquire();
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 64u, 0u);
    }
}

TEST(PoolAllocator, AvailableAndUsedAccounting) {
    PoolAllocator pool(sizeof(int), alignof(int), 4);
    EXPECT_EQ(pool.available(), 4u);
    EXPECT_EQ(pool.used(), 0u);
    void* a = pool.acquire();
    EXPECT_EQ(pool.used(), 1u);
    EXPECT_EQ(pool.available(), 3u);
    pool.release(a);
    EXPECT_EQ(pool.used(), 0u);
}

TEST(TypedPool, CreatesAndDestroysObjects) {
    struct Counter {
        static int& count() { static int n = 0; return n; }
        Counter()  { ++count(); }
        ~Counter() { --count(); }
    };
    Counter::count() = 0;
    {
        TypedPool<Counter> pool(8);
        Counter* a = pool.create();
        Counter* b = pool.create();
        EXPECT_EQ(Counter::count(), 2);
        pool.destroy(a);
        EXPECT_EQ(Counter::count(), 1);
        pool.destroy(b);
        EXPECT_EQ(Counter::count(), 0);
    }
}

TEST(TypedPool, CapacityExposed) {
    TypedPool<int> pool(7);
    EXPECT_EQ(pool.capacity(), 7u);
    EXPECT_EQ(pool.used(), 0u);
    EXPECT_EQ(pool.available(), 7u);
}
