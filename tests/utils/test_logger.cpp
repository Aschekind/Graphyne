#include <gtest/gtest.h>

#include "utils/logger.h"

using gn::utils::Logger;
using gn::utils::LogLevel;

TEST(Logger, InitializeIdempotent) {
    auto& l = Logger::instance();
    EXPECT_TRUE(l.initialize("", LogLevel::Trace, /*to_console=*/false));
    EXPECT_TRUE(l.initialized());
    // Second call should be a no-op but still return true.
    EXPECT_TRUE(l.initialize("", LogLevel::Debug, /*to_console=*/false));
    EXPECT_TRUE(l.initialized());
}

TEST(Logger, SetLevelChangesActiveLevel) {
    auto& l = Logger::instance();
    l.initialize("", LogLevel::Info, /*to_console=*/false);
    l.set_level(LogLevel::Warning);
    EXPECT_EQ(l.level(), LogLevel::Warning);
    // Macros should compile and not throw.
    GN_INFO("info {}", 1);
    GN_WARNING("warn {}", 2);
    GN_ERROR("err {}", 3);
    SUCCEED();
}
