#include <OpenLoco/Core/Numerics.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco;

TEST(NumericTest, bitScanForward)
{
    EXPECT_EQ(Numerics::bitScanForward(0), -1);
    EXPECT_EQ(Numerics::bitScanForward(0b00000001), 0);
    EXPECT_EQ(Numerics::bitScanForward(0b00000010), 1);

    EXPECT_EQ(Numerics::bitScanForward(0b10000010), 1);

    EXPECT_EQ(Numerics::bitScanForward(1u << 31), 31);
}

TEST(NumericTest, bitScanReverse)
{
    EXPECT_EQ(Numerics::bitScanReverse(0), -1);
    EXPECT_EQ(Numerics::bitScanReverse(0b00000001), 0);
    EXPECT_EQ(Numerics::bitScanReverse(0b00000010), 1);

    EXPECT_EQ(Numerics::bitScanReverse(0b10000010), 7);

    EXPECT_EQ(Numerics::bitScanReverse(1u << 31), 31);
}

static_assert(Numerics::setMask(0, 0b1100, true) == 0b1100);
static_assert(Numerics::setMask(0, 0b1100, false) == 0);
static_assert(Numerics::setMask(0b0110, 0b1100, false) == 0b0010);
static_assert(Numerics::setMask(0b0110, 0b1100, true) == 0b1110);
