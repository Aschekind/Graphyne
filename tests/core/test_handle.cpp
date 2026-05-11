#include <gtest/gtest.h>

#include "core/handle.h"

using gn::Handle;

namespace {
struct TestTag;
using TH = Handle<TestTag>;
} // namespace

TEST(Handle, DefaultIsInvalid) {
    TH h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.packed, 0u);
    EXPECT_FALSE(static_cast<bool>(h));
}

TEST(Handle, PacksIndexAndGeneration) {
    TH h{5, 3};
    EXPECT_EQ(h.index(), 5u);
    EXPECT_EQ(h.generation(), 3u);
    EXPECT_TRUE(h.valid());
}

TEST(Handle, MasksApplied) {
    // Generation > kGenMask should be masked.
    TH h{1, TH::kGenMask + 7};
    EXPECT_EQ(h.generation(), 6u);  // (kGenMask + 7) & kGenMask = 6
    EXPECT_EQ(h.index(), 1u);
}

TEST(Handle, OrderingAndEquality) {
    TH a{1, 0};
    TH b{1, 0};
    TH c{2, 0};
    EXPECT_EQ(a, b);
    EXPECT_LT(a, c);
    EXPECT_NE(a, c);
}

TEST(Handle, HashUsableInUnorderedContainer) {
    std::hash<TH> hasher;
    TH a{5, 1};
    TH b{5, 1};
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(Handle, InvalidFactory) {
    auto h = TH::invalid();
    EXPECT_FALSE(h.valid());
}
