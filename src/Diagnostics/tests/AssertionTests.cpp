#include <OpenLoco/Platform/Debug.h>

// Redefine the macro to ensure it calls std::abort() instead of platform-specific debug break,
// so that it can be properly caught by GTests death tests.
#undef OPENLOCO_DEBUG_BREAK
#define OPENLOCO_DEBUG_BREAK() std::abort()

#include <OpenLoco/Diagnostics/Assert.h>
#include <cstdlib>
#include <gtest/gtest.h>

using namespace OpenLoco;
using namespace OpenLoco::Diagnostics;

struct TestPoint
{
    int x, y;
    auto operator<=>(const TestPoint&) const = default;
};

TEST(AssertionsTest, AllSuccessCases)
{
    Assert::eq(42, 42);
    Assert::eq(3.14, 3.14);
    Assert::eq(std::string("hello"), "hello");
    Assert::eq(TestPoint{ 1, 2 }, TestPoint{ 1, 2 });

    Assert::neq(1, 2);
    Assert::neq(TestPoint{ 1, 2 }, TestPoint{ 3, 4 });

    Assert::lt(1, 10);
    Assert::le(5, 5);
    Assert::gt(10, 1);
    Assert::ge(7, 7);
    Assert::lt(TestPoint{ 1, 2 }, TestPoint{ 1, 3 });

    Assert::isTrue(true);
    Assert::isTrue(2 + 2 == 4);
    Assert::isFalse(false);

    Assert::isNull(nullptr);
    Assert::isNull(static_cast<int*>(nullptr));

    Assert::notNull(reinterpret_cast<void*>(0x1234));
    Assert::notNull(new int(42)); // raw pointer
}

enum class TestEnum
{
    ValueA = 1,
    ValueB = 2
};

TEST(AssertionsTest, EnumClass)
{
    EXPECT_DEATH(Assert::eq(TestEnum::ValueA, TestEnum::ValueB), "enum TestEnum::1 == enum TestEnum::2");
}

TEST(AssertionsDeathTest, FailingEq)
{
    EXPECT_DEATH(Assert::eq(10, 20), "Assertion failure.*10 == 20");
}

TEST(AssertionsDeathTest, FailingNeq)
{
    EXPECT_DEATH(Assert::neq(99, 99), "Assertion failure.*99 != 99");
}

TEST(AssertionsDeathTest, FailingLt)
{
    EXPECT_DEATH(Assert::lt(100, 50), "Assertion failure.*100 < 50");
}

TEST(AssertionsDeathTest, FailingLe)
{
    EXPECT_DEATH(Assert::le(10, 5), "Assertion failure.*10 <= 5");
}

TEST(AssertionsDeathTest, FailingGt)
{
    EXPECT_DEATH(Assert::gt(5, 10), "Assertion failure.*5 > 10");
}

TEST(AssertionsDeathTest, FailingGe)
{
    EXPECT_DEATH(Assert::ge(5, 10), "Assertion failure.*5 >= 10");
}

TEST(AssertionsDeathTest, FailingIsTrue)
{
    EXPECT_DEATH(Assert::isTrue(false), "expected true");
}

TEST(AssertionsDeathTest, FailingIsFalse)
{
    EXPECT_DEATH(Assert::isFalse(true), "expected false");
}

TEST(AssertionsDeathTest, FailingIsNull)
{
    int value = 42;
    EXPECT_DEATH(Assert::isNull(&value), "expected null pointer");
}

TEST(AssertionsDeathTest, FailingNotNull)
{
    EXPECT_DEATH(Assert::notNull(nullptr), "expected non-null pointer");
}

TEST(AssertionsTest, SmartPointerNullChecks)
{
    std::unique_ptr<int> empty;
    std::shared_ptr<int> sharedEmpty;
    auto valid = std::make_unique<int>(123);

    Assert::isNull(empty);
    Assert::isNull(sharedEmpty);
    Assert::notNull(valid);
}
