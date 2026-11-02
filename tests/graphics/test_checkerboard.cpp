#include <gtest/gtest.h>

#include "graphics/checkerboard.h"

using gn::graphics::make_checkerboard;
using gn::graphics::Rgba8;

namespace {
constexpr Rgba8 kWhite{255, 255, 255, 255};
constexpr Rgba8 kBlack{0, 0, 0, 255};

Rgba8 pixel_at(const std::vector<gn::u8>& px, gn::u32 width, gn::u32 x, gn::u32 y) {
    const size_t i = (static_cast<size_t>(y) * width + x) * 4;
    return {px[i], px[i + 1], px[i + 2], px[i + 3]};
}
} // namespace

TEST(Checkerboard, HasFourBytesPerPixel) {
    EXPECT_EQ(make_checkerboard(8, 4, 1, kWhite, kBlack).size(), 8u * 4u * 4u);
}

TEST(Checkerboard, AlternatesPerCell) {
    const auto px = make_checkerboard(4, 4, 2, kWhite, kBlack);
    EXPECT_EQ(pixel_at(px, 4, 0, 0), kWhite);
    EXPECT_EQ(pixel_at(px, 4, 1, 1), kWhite);
    EXPECT_EQ(pixel_at(px, 4, 2, 0), kBlack);
    EXPECT_EQ(pixel_at(px, 4, 0, 2), kBlack);
    EXPECT_EQ(pixel_at(px, 4, 3, 3), kWhite);
}

TEST(Checkerboard, ZeroCellIsTreatedAsOne) {
    const auto px = make_checkerboard(2, 1, 0, kWhite, kBlack);
    EXPECT_EQ(pixel_at(px, 2, 1, 0), kBlack);
}
