#include "core/clock.h"

namespace gn {

Clock::Clock() {
    reset();
}

void Clock::reset() {
    m_start       = HighResClock::now();
    m_last_tick   = m_start;
    m_first_tick  = true;
    m_last_delta  = 0.0f;
}

f32 Clock::tick() {
    const TimePoint now = HighResClock::now();
    if (m_first_tick) {
        m_first_tick = false;
        m_last_tick  = now;
        m_last_delta = 0.0f;
        return 0.0f;
    }
    const auto delta = std::chrono::duration_cast<std::chrono::duration<f32>>(now - m_last_tick).count();
    m_last_tick  = now;
    m_last_delta = delta;
    return delta;
}

f64 Clock::elapsed() const {
    const TimePoint now = HighResClock::now();
    return std::chrono::duration_cast<std::chrono::duration<f64>>(now - m_start).count();
}

f32 Clock::fps() const {
    return m_last_delta > 0.0f ? (1.0f / m_last_delta) : 0.0f;
}

} // namespace gn
