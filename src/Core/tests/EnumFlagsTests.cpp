#include <OpenLoco/Core/EnumFlags.hpp>
#include <gtest/gtest.h>

enum class TestFlags : std::uint32_t
{
    none = 0,
    a = 1U << 0,
    b = 1U << 2,
    c = 1U << 4,
    d = 1U << 8,
    e = 1U << 16,
    f = 1U << 24,
    g = 1U << 31,
};
OPENLOCO_ENABLE_ENUM_OPERATORS(TestFlags);

TEST(EnumFlagsTest, operatorOr)
{
    constexpr TestFlags flags = TestFlags::a | TestFlags::c | TestFlags::d | TestFlags::g;
    constexpr auto value = static_cast<std::underlying_type_t<TestFlags>>(flags);
    ASSERT_EQ(value, 0b1000'0000'0000'0000'0000'0001'0001'0001);
}

TEST(EnumFlagsTest, operatorAnd)
{
    constexpr TestFlags flags = (TestFlags::a | TestFlags::c | TestFlags::d | TestFlags::g) & TestFlags::c;
    constexpr auto value = static_cast<std::underlying_type_t<TestFlags>>(flags);
    ASSERT_EQ(value, 0b0000'0000'0000'0000'0000'0000'0001'0000);
}

TEST(EnumFlagsTest, operatorXor)
{
    constexpr TestFlags flags = (TestFlags::a | TestFlags::c | TestFlags::d) ^ (TestFlags::a | TestFlags::c | TestFlags::d) ^ TestFlags::f;
    constexpr auto value = static_cast<std::underlying_type_t<TestFlags>>(flags);
    ASSERT_EQ(value, 0b0000'0001'0000'0000'0000'0000'0000'0000);
}

TEST(EnumFlagsTest, operatorOrInplace)
{
    TestFlags flags = TestFlags::none;
    flags |= TestFlags::a;
    flags |= TestFlags::c;
    flags |= TestFlags::d;
    flags |= TestFlags::g;
    auto value = static_cast<std::underlying_type_t<TestFlags>>(flags);
    ASSERT_EQ(value, 0b1000'0000'0000'0000'0000'0001'0001'0001);
}

TEST(EnumFlagsTest, operatorAndInplace)
{
    TestFlags flags = (TestFlags::a | TestFlags::c | TestFlags::d | TestFlags::g);
    flags &= TestFlags::c;
    auto value = static_cast<std::underlying_type_t<TestFlags>>(flags);
    ASSERT_EQ(value, 0b0000'0000'0000'0000'0000'0000'0001'0000);
}

TEST(EnumFlagsTest, operatorXorInplace)
{
    TestFlags flags = (TestFlags::a | TestFlags::c | TestFlags::d);
    flags ^= (TestFlags::a | TestFlags::c | TestFlags::d);
    flags ^= TestFlags::f;
    auto value = static_cast<std::underlying_type_t<TestFlags>>(flags);
    ASSERT_EQ(value, 0b0000'0001'0000'0000'0000'0000'0000'0000);
}
