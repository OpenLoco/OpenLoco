#include <OpenLoco/Utility/Numeric.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco;

TEST(NumericTest, bitScanForward)
{
    EXPECT_EQ(Utility::bitScanForward(0), -1);
    EXPECT_EQ(Utility::bitScanForward(0b00000001), 0);
    EXPECT_EQ(Utility::bitScanForward(0b00000010), 1);

    EXPECT_EQ(Utility::bitScanForward(0b10000010), 1);

    EXPECT_EQ(Utility::bitScanForward(1u << 31), 31);
}

TEST(NumericTest, bitScanReverse)
{
    EXPECT_EQ(Utility::bitScanReverse(0), -1);
    EXPECT_EQ(Utility::bitScanReverse(0b00000001), 0);
    EXPECT_EQ(Utility::bitScanReverse(0b00000010), 1);

    EXPECT_EQ(Utility::bitScanReverse(0b10000010), 7);

    EXPECT_EQ(Utility::bitScanReverse(1u << 31), 31);
}

TEST(NumericTest, rol)
{
    // rol is constexpr so all of the following could be done as static_asserts

    EXPECT_EQ(Utility::rol(0u, 4), 0u);
    EXPECT_EQ(Utility::rol(0b1u, 0), 1u);
    EXPECT_EQ(Utility::rol(0b0010u, 1), 0b0100u);
    EXPECT_EQ(Utility::rol(0b0010u, 2), 0b1000u);
    EXPECT_EQ(Utility::rol(0b0101'0101'0101'0101u, 1), 0b1010'1010'1010'1010u);

    // Rollsover from 32 to 0.
    EXPECT_EQ(Utility::rol(0b0101'0101'0101'0101u, 32), 0b0101'0101'0101'0101u);
    EXPECT_EQ(Utility::rol(0b0101'0101'0101'0101u, 33), 0b1010'1010'1010'1010u);

    EXPECT_EQ(Utility::rol(1ULL, 62), 1ULL << 62);
}

TEST(NumericTest, ror)
{
    // ror is constexpr so all of the following could be done as static_asserts

    EXPECT_EQ(Utility::ror(0u, 4), 0);
    EXPECT_EQ(Utility::ror(0b1u, 0), 1);
    EXPECT_EQ(Utility::ror(0b00000010u, 1), 1);
    EXPECT_EQ(Utility::ror(0b00000010u, 2), 1u << 31);
    EXPECT_EQ(Utility::ror(0b0101'0101'0101'0101'0101'0101'0101'0101u, 1), 0b1010'1010'1010'1010'1010'1010'1010'1010u);

    // Rollsover from 32 to 0.
    EXPECT_EQ(Utility::ror(0b0101'0101'0101'0101'0101'0101'0101'0101u, 32), 0b0101'0101'0101'0101'0101'0101'0101'0101u);
    EXPECT_EQ(Utility::ror(0b0101'0101'0101'0101'0101'0101'0101'0101u, 33), 0b1010'1010'1010'1010'1010'1010'1010'1010u);

    EXPECT_EQ(Utility::ror(1ULL << 62, 62), 1u);
}

static_assert(Utility::setMask(0, 0b1100, true) == 0b1100);
static_assert(Utility::setMask(0, 0b1100, false) == 0);
static_assert(Utility::setMask(0b0110, 0b1100, false) == 0b0010);
static_assert(Utility::setMask(0b0110, 0b1100, true) == 0b1110);
