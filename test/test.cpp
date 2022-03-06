#include <gtest/gtest.h>

namespace Testing
{
    TEST(TestCaseName, TestName)
    {
        EXPECT_EQ(1, 1);
        EXPECT_TRUE(true);
        ASSERT_TRUE(true);
    }
}
