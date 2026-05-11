#include <gtest/gtest.h>

#include "memory/arena.h"

#include <cstdint>

using gn::memory::Arena;

TEST(Arena, AllocateReturnsAlignedPointer) {
    Arena a(1024);
    void* p = a.allocate(64, 16);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 16u, 0u);
}

TEST(Arena, UsedAndPeakTrack) {
    Arena a(1024);
    EXPECT_EQ(a.used(), 0u);
    a.allocate(100, 8);
    EXPECT_GE(a.used(), 100u);
    const auto peak1 = a.peak();
    a.allocate(50, 8);
    EXPECT_GE(a.peak(), peak1);
}

TEST(Arena, ResetRewindsButPreservesPeak) {
    Arena a(1024);
    a.allocate(500, 8);
    const auto peak_before = a.peak();
    a.reset();
    EXPECT_EQ(a.used(), 0u);
    EXPECT_EQ(a.peak(), peak_before);    // peak survives reset
    void* p = a.allocate(200, 8);
    EXPECT_NE(p, nullptr);
}

TEST(Arena, ExhaustionReturnsNull) {
    Arena a(64);
    EXPECT_NE(a.allocate(32, 8), nullptr);
    EXPECT_NE(a.allocate(16, 8), nullptr);
    EXPECT_EQ(a.allocate(64, 8), nullptr);  // would overflow
}

TEST(Arena, ZeroSizeOrBadAlignmentReturnsNull) {
    Arena a(64);
    EXPECT_EQ(a.allocate(0, 8), nullptr);
    EXPECT_EQ(a.allocate(8, 3), nullptr);    // 3 is not power-of-two
}

TEST(Arena, ZeroCapacityFailsGracefully) {
    Arena a(0);
    EXPECT_EQ(a.allocate(1, 1), nullptr);
}

TEST(Arena, CreateConstructsObjectInPlace) {
    struct Foo { int x; explicit Foo(int v) : x(v) {} };
    Arena a(256);
    Foo* f = a.create<Foo>(42);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->x, 42);
    f->~Foo();
}

TEST(Arena, MoveConstructorTransfersOwnership) {
    Arena a(128);
    a.allocate(32, 8);
    Arena b(std::move(a));
    EXPECT_EQ(a.capacity(), 0u);
    EXPECT_GE(b.used(), 32u);
}

TEST(Arena, RespectsAlignment64) {
    Arena a(4096);
    void* p = a.allocate(8, 64);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 64u, 0u);
}
