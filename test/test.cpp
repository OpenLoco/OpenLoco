#include <OpenLoco/Date.h>
#include <gtest/gtest.h>

namespace Testing
{
    TEST(TestIsLeapYear, DateTests)
    {
        ASSERT_FALSE(OpenLoco::isLeapYear(-1));
        ASSERT_FALSE(OpenLoco::isLeapYear(1));
        ASSERT_FALSE(OpenLoco::isLeapYear(2001));

        ASSERT_TRUE(OpenLoco::isLeapYear(0));
        ASSERT_TRUE(OpenLoco::isLeapYear(2000));
        ASSERT_TRUE(OpenLoco::isLeapYear(2004));
        ASSERT_TRUE(OpenLoco::isLeapYear(1900));
        ASSERT_TRUE(OpenLoco::isLeapYear(-4));
    }
}
