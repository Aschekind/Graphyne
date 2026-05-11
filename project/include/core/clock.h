/**
 * @file core/clock.h
 * @brief A monotonic clock that returns delta time per frame.
 */
#pragma once

#include "core/types.h"

#include <chrono>

namespace gn {

class Clock {
public:
    using HighResClock = std::chrono::steady_clock;
    using TimePoint    = HighResClock::time_point;

    Clock();

    /// Advance the clock and return seconds elapsed since the previous tick().
    /// The first call returns 0.
    f32 tick();

    /// Wall-clock seconds since construction (independent of tick()).
    f64 elapsed() const;

    /// Frames-per-second computed from the last delta (>0 when delta>0).
    f32 fps() const;

    /// Last delta returned by tick(), in seconds.
    f32 delta_seconds() const { return m_last_delta; }

    /// Reset the clock so the next tick() returns 0 again.
    void reset();

private:
    TimePoint m_start;
    TimePoint m_last_tick;
    bool      m_first_tick;
    f32       m_last_delta;
};

} // namespace gn
