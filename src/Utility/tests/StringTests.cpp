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

TEST(StringTests, strlcpy)
{
    constexpr size_t kBufferLength = 10;
    constexpr size_t kSmallLength = 5;
    char buffer[kBufferLength]{};

    // Check handles nulls
    auto len = Utility::strlcpy(buffer, "", kBufferLength);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check handles nulls
    len = Utility::strlcpy(buffer, "", 0);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check handles nulls
    len = Utility::strlcpy(nullptr, "", kBufferLength);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check handles nulls
    len = Utility::strlcpy(buffer, nullptr, kBufferLength);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check that it copies
    len = Utility::strlcpy(buffer, "0123456", kBufferLength);
    EXPECT_STREQ(buffer, "0123456");
    EXPECT_EQ(len, 7);

    std::fill(std::begin(buffer), std::end(buffer), '\0');

    // Check that it truncates
    len = Utility::strlcpy(buffer, "012345678", kSmallLength);
    EXPECT_STREQ(buffer, "0123");
    EXPECT_EQ(len, 4);

    // Check that it null terminates
    std::fill(std::begin(buffer), std::end(buffer), 'a');
    len = Utility::strlcpy(buffer, "Hello!", kBufferLength);
    EXPECT_STREQ(buffer, "Hello!");
    EXPECT_EQ(len, 6);
}

TEST(StringTests, strlcat)
{
    constexpr size_t kBufferLength = 20;
    constexpr size_t kSmallLength = 5;
    char buffer[kBufferLength]{};

    // Check handles nulls
    auto len = Utility::strlcpy(buffer, "", kBufferLength);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check handles nulls
    len = Utility::strlcpy(buffer, "", 0);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check handles nulls
    len = Utility::strlcpy(nullptr, "", kBufferLength);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check handles nulls
    len = Utility::strlcpy(buffer, nullptr, kBufferLength);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(len, 0);

    // Check that it copies
    len = Utility::strlcat(buffer, "0123456", kBufferLength);
    EXPECT_STREQ(buffer, "0123456");
    EXPECT_EQ(len, 7);

    std::fill(std::begin(buffer), std::end(buffer), '\0');

    // Check that it truncates
    len = Utility::strlcat(buffer, "012345678", kSmallLength);
    EXPECT_STREQ(buffer, "0123");
    EXPECT_EQ(len, kSmallLength - 1);

    std::fill(std::begin(buffer), std::end(buffer), '\0');
    len = Utility::strlcpy(buffer, "Hello ", kBufferLength);

    // Check that it concats
    len = Utility::strlcat(buffer, "World!", kBufferLength);
    EXPECT_STREQ(buffer, "Hello World!");
    EXPECT_EQ(len, 12);

    // Check that it fails to concat if buffer really small
    len = Utility::strlcat(buffer, "World!", kSmallLength);
    EXPECT_STREQ(buffer, "Hello World!");
    EXPECT_EQ(len, kSmallLength - 1);

    // Check that it truncats concat if buffer small
    len = Utility::strlcat(buffer, "1234", sizeof("Hello World!") + 3);
    EXPECT_STREQ(buffer, "Hello World!123");
    EXPECT_EQ(len, 15);
}

TEST(StringTests, split)
{
    const auto split1 = Utility::split("hello world foo bar", " ");
    EXPECT_EQ(split1.size(), 4);
    EXPECT_EQ(split1[0], "hello");
    EXPECT_EQ(split1[1], "world");
    EXPECT_EQ(split1[2], "foo");
    EXPECT_EQ(split1[3], "bar");

    const auto split2 = Utility::split("helloSEPworldSEPfooSEPbarSEP", "SEP");
    EXPECT_EQ(split2.size(), 4);
    EXPECT_EQ(split2[0], "hello");
    EXPECT_EQ(split2[1], "world");
    EXPECT_EQ(split2[2], "foo");
    EXPECT_EQ(split2[3], "bar");

    const auto split3 = Utility::split(" foo", " ");
    EXPECT_EQ(split3.size(), 1);
    EXPECT_EQ(split3[0], "foo");

    const auto split4 = Utility::split("", " ");
    EXPECT_EQ(split4.size(), 0);
}

TEST(StringTests, trim)
{
    EXPECT_EQ(Utility::trim(" hello world"), "hello world");
    EXPECT_EQ(Utility::trim("   hello world   "), "hello world");
    EXPECT_EQ(Utility::trim("\thello world\r\n"), "hello world");
    EXPECT_EQ(Utility::trim(" "), "");
    EXPECT_EQ(Utility::trim(""), "");
}
