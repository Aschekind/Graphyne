#include <gtest/gtest.h>

#include "core/clock.h"

#include <chrono>
#include <thread>

using gn::Clock;

TEST(Clock, FirstTickReturnsZero) {
    Clock c;
    EXPECT_EQ(c.tick(), 0.0f);
}

TEST(Clock, SubsequentTickIsPositive) {
    Clock c;
    c.tick();                                      // prime
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const float dt = c.tick();
    EXPECT_GT(dt, 0.0f);
    EXPECT_LT(dt, 1.0f);                           // sanity bound
}

TEST(Clock, DeltaSecondsMatchesLastTick) {
    Clock c;
    c.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const float dt = c.tick();
    EXPECT_FLOAT_EQ(c.delta_seconds(), dt);
}

TEST(Clock, FpsIsReciprocalOfDelta) {
    Clock c;
    c.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    c.tick();
    // fps should equal 1 / delta within float precision.
    EXPECT_NEAR(c.fps(), 1.0f / c.delta_seconds(), 1e-3f);
}

TEST(Clock, ResetRestartsTimer) {
    Clock c;
    c.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    c.tick();
    c.reset();
    EXPECT_EQ(c.tick(), 0.0f);  // first tick after reset is 0
}

TEST(Clock, ElapsedIsMonotonic) {
    Clock c;
    const double e1 = c.elapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const double e2 = c.elapsed();
    EXPECT_GE(e2, e1);
}
