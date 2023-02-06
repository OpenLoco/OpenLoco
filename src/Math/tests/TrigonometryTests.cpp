#include <OpenLoco/Math/Trigonometry.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco::Math;

TEST(TrigonometryTest, integerSinePrecisionHigh)
{
    ASSERT_EQ(Trigonometry::integerSinePrecisionHigh(0, 500), 0);
    ASSERT_EQ(Trigonometry::integerSinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh / 4, 500), 499);
    ASSERT_EQ(Trigonometry::integerSinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh / 2, 500), 0);
    ASSERT_EQ(Trigonometry::integerSinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh / 4 + Trigonometry::kDirectionPrecisionHigh / 2, 500), -499);
    ASSERT_EQ(Trigonometry::integerSinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh, 500), 0);
}

TEST(TrigonometryTest, integerCosinePrecisionHigh)
{
    ASSERT_EQ(Trigonometry::integerCosinePrecisionHigh(0, 500), 499);
    ASSERT_EQ(Trigonometry::integerCosinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh / 4, 500), 0);
    ASSERT_EQ(Trigonometry::integerCosinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh / 2, 500), -499);
    ASSERT_EQ(Trigonometry::integerCosinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh / 4 + Trigonometry::kDirectionPrecisionHigh / 2, 500), 0);
    ASSERT_EQ(Trigonometry::integerCosinePrecisionHigh(Trigonometry::kDirectionPrecisionHigh, 500), 499);
}
