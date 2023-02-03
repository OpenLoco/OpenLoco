#include <OpenLoco/Math/Vector.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco::Math;

using Point2D = Vector::TVector2<int32_t, 1>;
using Point3D = Vector::TVector3<int32_t>;
TEST(VectorTest, fastSquareRoot)
{
    ASSERT_EQ(Vector::fastSquareRoot(0), 0);
    ASSERT_EQ(Vector::fastSquareRoot(256), 16);
    ASSERT_EQ(Vector::fastSquareRoot(10000), 100);
    ASSERT_EQ(Vector::fastSquareRoot(0xFFFFFFFF), 65519); // Note: Not very accurate...
    ASSERT_EQ(Vector::fastSquareRoot(4095), 63);          // Note: Not very accurate...
}

// This test is ultimately the same as fastSquareRoot but more how we would acutally use it
TEST(VectorTest, distance)
{
    Point2D a{ 10, 10 };
    Point2D b{ 15, 15 };

    ASSERT_EQ(Vector::distance(a, b), 7);
    a = { 0, 0 };
    b = { 0, 0 };
    ASSERT_EQ(Vector::distance(a, b), 0);
    a = { -10, -10 };
    b = { 0, 0 };
    ASSERT_EQ(Vector::distance(a, b), 14);
    a = { 0, 0 };
    b = { -10, -10 };
    ASSERT_EQ(Vector::distance(a, b), 14);
    a = { -10, -10 };
    b = { -10, -10 };
    ASSERT_EQ(Vector::distance(a, b), 0);
    a = { 10, 10 };
    b = { 10, 10 };
    ASSERT_EQ(Vector::distance(a, b), 0);
}

static_assert(Vector::rotate(Point2D{ 10, 10 }, 0) == Point2D{ 10, 10 });
static_assert(Vector::rotate(Point2D{ 10, 10 }, 1) == Point2D{ 10, -10 });
static_assert(Vector::rotate(Point2D{ 10, 10 }, 2) == Point2D{ -10, -10 });
static_assert(Vector::rotate(Point2D{ 10, 10 }, 3) == Point2D{ -10, 10 });
// Rolls over
static_assert(Vector::rotate(Point2D{ 10, 10 }, 4) == Point2D{ 10, 10 });
// Rolls under
static_assert(Vector::rotate(Point2D{ 10, 10 }, -1) == Point2D{ -10, 10 });

TEST(VectorTest, manhattenDistance)
{
    Point2D a{ 10, 10 };
    Point2D b{ 15, 15 };

    ASSERT_EQ(Vector::manhattanDistance(a, b), 10);
    a = { 0, 0 };
    b = { 0, 0 };
    ASSERT_EQ(Vector::manhattanDistance(a, b), 0);
    a = { -10, -10 };
    b = { 0, 0 };
    ASSERT_EQ(Vector::manhattanDistance(a, b), 20);
    a = { 0, 0 };
    b = { -10, -10 };
    ASSERT_EQ(Vector::manhattanDistance(a, b), 20);
    a = { -10, -10 };
    b = { -10, -10 };
    ASSERT_EQ(Vector::manhattanDistance(a, b), 0);
    a = { 10, 10 };
    b = { 10, 10 };
    ASSERT_EQ(Vector::manhattanDistance(a, b), 0);
}
