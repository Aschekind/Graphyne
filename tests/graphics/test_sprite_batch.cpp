#include <gtest/gtest.h>

#include "graphics/sprite_batch.h"

using gn::TextureHandle;
using gn::vec2;
using gn::graphics::Sprite;
using gn::graphics::SpriteBatch;

namespace {
const TextureHandle kTexA{1, 0};
const TextureHandle kTexB{2, 0};

Sprite sprite(TextureHandle tex, gn::i32 layer = 0, vec2 position = {0.0f, 0.0f}) {
    Sprite s;
    s.texture  = tex;
    s.layer    = layer;
    s.position = position;
    return s;
}

void expect_near(vec2 a, vec2 b) {
    EXPECT_NEAR(a.x, b.x, 1e-5f);
    EXPECT_NEAR(a.y, b.y, 1e-5f);
}
} // namespace

TEST(SpriteBatch, QuadCornersWithTopLeftPivot) {
    SpriteBatch batch;
    Sprite      s = sprite(kTexA, 0, {10.0f, 20.0f});
    s.size        = {4.0f, 2.0f};
    batch.draw(s);
    batch.finish();
    ASSERT_EQ(batch.vertices().size(), 4u);
    expect_near(batch.vertices()[0].pos, {10.0f, 20.0f}); // top-left
    expect_near(batch.vertices()[1].pos, {14.0f, 20.0f}); // top-right
    expect_near(batch.vertices()[2].pos, {10.0f, 22.0f}); // bottom-left
    expect_near(batch.vertices()[3].pos, {14.0f, 22.0f});
}

TEST(SpriteBatch, PivotMovesTheQuad) {
    SpriteBatch batch;
    Sprite      s = sprite(kTexA, 0, {10.0f, 10.0f});
    s.size        = {4.0f, 2.0f};
    s.pivot       = {0.5f, 0.5f};
    batch.draw(s);
    expect_near(batch.vertices()[0].pos, {8.0f, 9.0f});
    expect_near(batch.vertices()[3].pos, {12.0f, 11.0f});
}

TEST(SpriteBatch, RotatesAroundThePivot) {
    SpriteBatch batch;
    Sprite      s = sprite(kTexA, 0, {10.0f, 10.0f});
    s.size        = {2.0f, 2.0f};
    s.rotation    = glm::half_pi<float>(); // clockwise on a y-down screen
    batch.draw(s);
    // The top-left pivot stays; the top-right corner swings down, the bottom-left swings left.
    expect_near(batch.vertices()[0].pos, {10.0f, 10.0f});
    expect_near(batch.vertices()[1].pos, {10.0f, 12.0f});
    expect_near(batch.vertices()[2].pos, {8.0f, 10.0f});
    expect_near(batch.vertices()[3].pos, {8.0f, 12.0f});
}

TEST(SpriteBatch, RotatesAroundACentrePivot) {
    SpriteBatch batch;
    Sprite      s = sprite(kTexA, 0, {10.0f, 10.0f});
    s.size        = {4.0f, 2.0f};
    s.pivot       = {0.5f, 0.5f};
    s.rotation    = glm::pi<float>();
    batch.draw(s);
    expect_near(batch.vertices()[0].pos, {12.0f, 11.0f}); // top-left lands at the bottom-right
    expect_near(batch.vertices()[3].pos, {8.0f, 9.0f});
}

TEST(SpriteBatch, UvRectAndTint) {
    SpriteBatch batch;
    Sprite      s = sprite(kTexA);
    s.uv          = {0.25f, 0.5f, 0.75f, 1.0f};
    s.tint        = gn::Color{0.1f, 0.2f, 0.3f, 0.4f};
    batch.draw(s);
    expect_near(batch.vertices()[0].uv, {0.25f, 0.5f});
    expect_near(batch.vertices()[1].uv, {0.75f, 0.5f});
    expect_near(batch.vertices()[2].uv, {0.25f, 1.0f});
    expect_near(batch.vertices()[3].uv, {0.75f, 1.0f});
    for (const auto& v : batch.vertices()) {
        EXPECT_FLOAT_EQ(v.color.r, 0.1f);
        EXPECT_FLOAT_EQ(v.color.a, 0.4f);
    }
}

TEST(SpriteBatch, ConsecutiveSameTextureMerges) {
    SpriteBatch batch;
    batch.draw(sprite(kTexA));
    batch.draw(sprite(kTexA));
    batch.draw(sprite(kTexA));
    batch.finish();
    ASSERT_EQ(batch.ranges().size(), 1u);
    EXPECT_EQ(batch.ranges()[0].texture, kTexA);
    EXPECT_EQ(batch.ranges()[0].first_index, 0u);
    EXPECT_EQ(batch.ranges()[0].index_count, 18u);
}

TEST(SpriteBatch, KeepsSubmissionOrderAndDoesNotMergeAcrossOtherTextures) {
    SpriteBatch batch;
    batch.draw(sprite(kTexA));
    batch.draw(sprite(kTexB));
    batch.draw(sprite(kTexB));
    batch.draw(sprite(kTexA));
    batch.finish();
    ASSERT_EQ(batch.ranges().size(), 3u);
    EXPECT_EQ(batch.ranges()[0].texture, kTexA);
    EXPECT_EQ(batch.ranges()[1].texture, kTexB);
    EXPECT_EQ(batch.ranges()[1].first_index, 6u);
    EXPECT_EQ(batch.ranges()[1].index_count, 12u);
    EXPECT_EQ(batch.ranges()[2].texture, kTexA);
    EXPECT_EQ(batch.ranges()[2].first_index, 18u);
}

TEST(SpriteBatch, LayersSortStably) {
    SpriteBatch batch;
    batch.draw(sprite(kTexA, 1, {1.0f, 0.0f}));
    batch.draw(sprite(kTexB, 0, {2.0f, 0.0f}));
    batch.draw(sprite(kTexA, 1, {3.0f, 0.0f}));
    batch.draw(sprite(kTexB, 0, {4.0f, 0.0f}));
    batch.finish();
    // Layer 0 first (B at x=2 then x=4), then layer 1 (A at x=1 then x=3).
    ASSERT_EQ(batch.ranges().size(), 2u);
    EXPECT_EQ(batch.ranges()[0].texture, kTexB);
    EXPECT_EQ(batch.ranges()[0].index_count, 12u);
    EXPECT_EQ(batch.ranges()[1].texture, kTexA);
    EXPECT_EQ(batch.ranges()[1].first_index, 12u);
    EXPECT_FLOAT_EQ(batch.vertices()[0].pos.x, 2.0f);
    EXPECT_FLOAT_EQ(batch.vertices()[4].pos.x, 4.0f);
    EXPECT_FLOAT_EQ(batch.vertices()[8].pos.x, 1.0f);
    EXPECT_FLOAT_EQ(batch.vertices()[12].pos.x, 3.0f);
}

TEST(SpriteBatch, DropsBeyondCapacity) {
    SpriteBatch batch;
    for (gn::u32 i = 0; i < SpriteBatch::kMaxSprites + 5; ++i) batch.draw(sprite(kTexA));
    batch.finish();
    EXPECT_EQ(batch.sprite_count(), SpriteBatch::kMaxSprites);
    EXPECT_EQ(batch.dropped(), 5u);
    EXPECT_EQ(batch.vertices().size(), SpriteBatch::kMaxSprites * 4u);
    ASSERT_EQ(batch.ranges().size(), 1u);
    EXPECT_EQ(batch.ranges()[0].index_count, SpriteBatch::kMaxSprites * 6u);
}

TEST(SpriteBatch, ClearStartsOver) {
    SpriteBatch batch;
    batch.draw(sprite(kTexA));
    batch.finish();
    batch.clear();
    batch.finish();
    EXPECT_EQ(batch.sprite_count(), 0u);
    EXPECT_TRUE(batch.vertices().empty());
    EXPECT_TRUE(batch.ranges().empty());
}
