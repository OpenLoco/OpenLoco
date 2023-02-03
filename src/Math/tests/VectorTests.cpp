#include <OpenLoco/Math/Vector.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco::Math;

TEST(VectorTest, fastSquareRoot)
{
    ASSERT_EQ(Vector::fastSquareRoot(0), 0);
}
