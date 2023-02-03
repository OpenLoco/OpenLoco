#include <OpenLoco/Math/Vector.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco::Math;

TEST(VectorTest, fastSquareRoot)
{
    ASSERT_EQ(Vector::fastSquareRoot(0), 0);
    ASSERT_EQ(Vector::fastSquareRoot(256), 16);
    ASSERT_EQ(Vector::fastSquareRoot(10000), 100);
    ASSERT_EQ(Vector::fastSquareRoot(0xFFFFFFFF), 65519); // Note: Not very accurate...
    ASSERT_EQ(Vector::fastSquareRoot(4095), 63);          // Note: Not very accurate...
}
