#include <gtest/gtest.h>

#include "core/slot_map.h"

#include <vector>

using gn::SlotMap;

namespace {
struct Thing { int value = 0; };
struct ThingTag;
using ThingMap = SlotMap<Thing, ThingTag>;
} // namespace

TEST(SlotMap, EmptyOnConstruction) {
    ThingMap m;
    EXPECT_EQ(m.size(), 0u);
    EXPECT_TRUE(m.empty());
}

TEST(SlotMap, InsertAndGet) {
    ThingMap m;
    auto h = m.insert({42});
    ASSERT_TRUE(h.valid());
    ASSERT_NE(m.get(h), nullptr);
    EXPECT_EQ(m.get(h)->value, 42);
    EXPECT_EQ(m.size(), 1u);
}

TEST(SlotMap, EraseInvalidatesHandle) {
    ThingMap m;
    auto h = m.insert({1});
    EXPECT_TRUE(m.contains(h));
    EXPECT_TRUE(m.erase(h));
    EXPECT_FALSE(m.contains(h));
    EXPECT_EQ(m.get(h), nullptr);
    EXPECT_EQ(m.size(), 0u);
}

TEST(SlotMap, RecyclesSlotWithNewGeneration) {
    ThingMap m;
    auto h1 = m.insert({10});
    m.erase(h1);
    auto h2 = m.insert({20});
    EXPECT_EQ(h1.index(), h2.index());                // same slot
    EXPECT_NE(h1.generation(), h2.generation());      // different generation
    EXPECT_FALSE(m.contains(h1));
    EXPECT_TRUE(m.contains(h2));
    EXPECT_EQ(m.get(h2)->value, 20);
}

TEST(SlotMap, MultipleInsertsAndIteration) {
    ThingMap m;
    std::vector<ThingMap::HandleT> handles;
    for (int i = 0; i < 5; ++i) handles.push_back(m.insert({i * 10}));
    EXPECT_EQ(m.size(), 5u);

    int sum = 0;
    for (const auto& t : m) sum += t.value;
    EXPECT_EQ(sum, 0 + 10 + 20 + 30 + 40);

    // All handles valid.
    for (const auto& h : handles) EXPECT_TRUE(m.contains(h));
}

TEST(SlotMap, EraseMiddlePreservesOthers) {
    ThingMap m;
    auto a = m.insert({1});
    auto b = m.insert({2});
    auto c = m.insert({3});
    EXPECT_TRUE(m.erase(b));
    EXPECT_TRUE(m.contains(a));
    EXPECT_FALSE(m.contains(b));
    EXPECT_TRUE(m.contains(c));
    EXPECT_EQ(m.get(a)->value, 1);
    EXPECT_EQ(m.get(c)->value, 3);
}

TEST(SlotMap, EraseWithStaleHandleFails) {
    ThingMap m;
    auto h = m.insert({1});
    EXPECT_TRUE(m.erase(h));
    EXPECT_FALSE(m.erase(h));   // double-erase
}

TEST(SlotMap, ClearResetsEverything) {
    ThingMap m;
    auto h = m.insert({5});
    m.clear();
    EXPECT_EQ(m.size(), 0u);
    EXPECT_FALSE(m.contains(h));
}

TEST(SlotMap, DefaultHandleNeverContained) {
    ThingMap m;
    ThingMap::HandleT h;
    EXPECT_FALSE(m.contains(h));
    EXPECT_EQ(m.get(h), nullptr);
}
