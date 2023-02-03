#include <gtest/gtest.h>
#include <OpenLoco/Math/Vector.hpp>

using namespace OpenLoco::Math;

TEST(VectorTest, fastSquareRoot)
{
    ASSERT_EQ(Vector::fastSquareRoot(0), 0);
}
