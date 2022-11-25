#include <OpenLoco/Utility/String.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco;

TEST(StringTests, iequals)
{
    EXPECT_TRUE(Utility::iequals("", ""));
    EXPECT_TRUE(Utility::iequals("Test", "Test"));
    EXPECT_TRUE(Utility::iequals("TesT", "Test"));
    EXPECT_TRUE(Utility::iequals("TEsT", "Test"));

    EXPECT_FALSE(Utility::iequals("Test", "Message"));
    EXPECT_FALSE(Utility::iequals("Test", "TestMessage"));
    EXPECT_FALSE(Utility::iequals("", "Test"));
    EXPECT_FALSE(Utility::iequals("Test", ""));
}

TEST(StringTests, equals)
{
    EXPECT_TRUE(Utility::equals("", "", true));
    EXPECT_TRUE(Utility::equals("Test", "Test", true));
    EXPECT_TRUE(Utility::equals("TesT", "Test", true));
    EXPECT_TRUE(Utility::equals("TEsT", "Test", true));

    EXPECT_FALSE(Utility::equals("Test", "Message", true));
    EXPECT_FALSE(Utility::equals("Test", "TestMessage", true));
    EXPECT_FALSE(Utility::equals("", "Test", true));
    EXPECT_FALSE(Utility::equals("Test", "", true));

    EXPECT_TRUE(Utility::equals("", ""));
    EXPECT_TRUE(Utility::equals("Test", "Test"));

    EXPECT_FALSE(Utility::equals("TesT", "Test"));
    EXPECT_FALSE(Utility::equals("TEsT", "Test"));
    EXPECT_FALSE(Utility::equals("Test", "Message"));
    EXPECT_FALSE(Utility::equals("Test", "TestMessage"));
    EXPECT_FALSE(Utility::equals("", "Test"));
    EXPECT_FALSE(Utility::equals("Test", ""));
}

TEST(StringTests, startsWith)
{
    EXPECT_TRUE(Utility::startsWith("", "", true));
    EXPECT_TRUE(Utility::startsWith("Test", "Test", true));
    EXPECT_TRUE(Utility::startsWith("TesT", "Test", true));
    EXPECT_TRUE(Utility::startsWith("TEsT", "Test", true));
    EXPECT_TRUE(Utility::startsWith("TestMessage", "Test", true));
    EXPECT_TRUE(Utility::startsWith("TESTMessage", "Test", true));
    EXPECT_TRUE(Utility::startsWith("Test", "", true));

    EXPECT_FALSE(Utility::startsWith("Test", "Message", true));
    EXPECT_FALSE(Utility::startsWith("", "Test", true));

    EXPECT_TRUE(Utility::startsWith("", ""));
    EXPECT_TRUE(Utility::startsWith("Test", "Test"));
    EXPECT_TRUE(Utility::startsWith("TestMessage", "Test"));
    EXPECT_TRUE(Utility::startsWith("Test", ""));

    EXPECT_FALSE(Utility::startsWith("TesT", "Test"));
    EXPECT_FALSE(Utility::startsWith("TEsT", "Test"));
    EXPECT_FALSE(Utility::startsWith("TESTMessaage", "Test"));
    EXPECT_FALSE(Utility::startsWith("Test", "Message"));
    EXPECT_FALSE(Utility::startsWith("", "Test"));
}

TEST(StringTests, endsWith)
{
    EXPECT_TRUE(Utility::endsWith("", "", true));
    EXPECT_TRUE(Utility::endsWith("Test", "Test", true));
    EXPECT_TRUE(Utility::endsWith("TesT", "Test", true));
    EXPECT_TRUE(Utility::endsWith("TEsT", "Test", true));
    EXPECT_TRUE(Utility::endsWith("TestMessage", "Message", true));
    EXPECT_TRUE(Utility::endsWith("TestMESSAGE", "Message", true));
    EXPECT_TRUE(Utility::endsWith("Test", "", true));

    EXPECT_FALSE(Utility::endsWith("Test", "Message", true));
    EXPECT_FALSE(Utility::endsWith("", "Test", true));

    EXPECT_TRUE(Utility::endsWith("", ""));
    EXPECT_TRUE(Utility::endsWith("Test", "Test"));
    EXPECT_TRUE(Utility::endsWith("TestMessage", "Message"));
    EXPECT_TRUE(Utility::endsWith("Test", ""));

    EXPECT_FALSE(Utility::endsWith("TesT", "Test"));
    EXPECT_FALSE(Utility::endsWith("TEsT", "Test"));
    EXPECT_FALSE(Utility::endsWith("TestMESSAGE", "Message"));
    EXPECT_FALSE(Utility::endsWith("Test", "Message"));
    EXPECT_FALSE(Utility::endsWith("", "Test"));
}
