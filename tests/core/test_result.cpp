#include <gtest/gtest.h>

#include "core/result.h"

#include <string>

using gn::Err;
using gn::Error;
using gn::Ok;
using gn::Result;

TEST(Result, HoldsValue) {
    Result<int> r(42);
    EXPECT_TRUE(r.ok());
    EXPECT_FALSE(r.err());
    EXPECT_EQ(r.value(), 42);
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(Result, HoldsError) {
    Result<int> r(Err{"failure"});
    EXPECT_FALSE(r.ok());
    EXPECT_TRUE(r.err());
    EXPECT_EQ(r.error().what(), "failure");
    EXPECT_FALSE(static_cast<bool>(r));
}

TEST(Result, ValueOr) {
    Result<int> ok(10);
    Result<int> bad(Err{"x"});
    EXPECT_EQ(ok.value_or(0), 10);
    EXPECT_EQ(bad.value_or(99), 99);
}

TEST(Result, VoidOk) {
    Result<void> r = Ok();
    EXPECT_TRUE(r.ok());
    EXPECT_FALSE(r.err());
}

TEST(Result, VoidError) {
    Result<void> r(Err{"boom"});
    EXPECT_FALSE(r.ok());
    EXPECT_TRUE(r.err());
    EXPECT_EQ(r.error().what(), "boom");
}

TEST(Result, MoveOnlyValue) {
    struct MoveOnly {
        int v;
        MoveOnly(int x) : v(x) {}
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        MoveOnly(const MoveOnly&) = delete;
    };
    Result<MoveOnly> r(MoveOnly{7});
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().v, 7);
}

TEST(Result, ErrorMessagePropagates) {
    auto make = []() -> Result<std::string> {
        return Err{"not found"};
    };
    auto r = make();
    EXPECT_TRUE(r.err());
    EXPECT_EQ(r.error().message, "not found");
}
