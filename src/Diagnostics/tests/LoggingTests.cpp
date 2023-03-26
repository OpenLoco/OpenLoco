#include <OpenLoco/Diagnostics/Logging.h>
#include <gtest/gtest.h>

using namespace OpenLoco;
using namespace OpenLoco::Diagnostics;

class TestLogLevelSink : public Logging::LogSink
{
    std::string _lastMessage;
    Logging::Level _lastLevel{};

public:
    void print(Logging::Level level, std::string_view msg) override
    {
        _lastLevel = level;
        _lastMessage = std::string{ msg };
    }

    std::string_view getLastMessage() const
    {
        return _lastMessage;
    }

    Logging::Level getLastLevel() const
    {
        return _lastLevel;
    }
};

TEST(LoggingTests, BasicSinkTest)
{
    auto testSink = std::make_shared<TestLogLevelSink>();
    Logging::installSink(testSink);

    Logging::info("TestInfo");
    ASSERT_EQ(testSink->getLastMessage(), "TestInfo");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::warn("TestWarning");
    ASSERT_EQ(testSink->getLastMessage(), "TestWarning");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::warning);

    Logging::error("TestError");
    ASSERT_EQ(testSink->getLastMessage(), "TestError");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::error);

    Logging::verbose("TestVerbose");
    ASSERT_EQ(testSink->getLastMessage(), "TestVerbose");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::verbose);

    Logging::removeSink(testSink);
}

TEST(LoggingTests, LevelFilterTestInfo)
{
    auto testSink = std::make_shared<TestLogLevelSink>();
    Logging::installSink(testSink);

    Logging::disableLevel(Logging::Level::info);

    Logging::info("TestInfo");
    ASSERT_EQ(testSink->getLastMessage(), "");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::enableLevel(Logging::Level::info);

    Logging::info("TestInfo");
    ASSERT_EQ(testSink->getLastMessage(), "TestInfo");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::warn("TestWarning");
    ASSERT_EQ(testSink->getLastMessage(), "TestWarning");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::warning);

    Logging::error("TestError");
    ASSERT_EQ(testSink->getLastMessage(), "TestError");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::error);

    Logging::verbose("TestVerbose");
    ASSERT_EQ(testSink->getLastMessage(), "TestVerbose");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::verbose);

    Logging::removeSink(testSink);
}

TEST(LoggingTests, LevelFilterTestWarning)
{
    auto testSink = std::make_shared<TestLogLevelSink>();
    Logging::installSink(testSink);

    Logging::info("TestInfo");
    ASSERT_EQ(testSink->getLastMessage(), "TestInfo");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::disableLevel(Logging::Level::warning);

    Logging::warn("TestWarning");
    ASSERT_EQ(testSink->getLastMessage(), "TestInfo");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::enableLevel(Logging::Level::warning);

    Logging::warn("TestWarning");
    ASSERT_EQ(testSink->getLastMessage(), "TestWarning");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::warning);

    Logging::error("TestError");
    ASSERT_EQ(testSink->getLastMessage(), "TestError");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::error);

    Logging::verbose("TestVerbose");
    ASSERT_EQ(testSink->getLastMessage(), "TestVerbose");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::verbose);

    Logging::removeSink(testSink);
}

TEST(LoggingTests, LevelFilterTestError)
{
    auto testSink = std::make_shared<TestLogLevelSink>();
    Logging::installSink(testSink);

    Logging::info("TestInfo");
    ASSERT_EQ(testSink->getLastMessage(), "TestInfo");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::warn("TestWarning");
    ASSERT_EQ(testSink->getLastMessage(), "TestWarning");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::warning);

    Logging::disableLevel(Logging::Level::error);

    Logging::error("TestError");
    ASSERT_EQ(testSink->getLastMessage(), "TestWarning");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::warning);

    Logging::enableLevel(Logging::Level::error);

    Logging::error("TestError");
    ASSERT_EQ(testSink->getLastMessage(), "TestError");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::error);

    Logging::verbose("TestVerbose");
    ASSERT_EQ(testSink->getLastMessage(), "TestVerbose");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::verbose);

    Logging::removeSink(testSink);
}

TEST(LoggingTests, LevelFilterTestVerbose)
{
    auto testSink = std::make_shared<TestLogLevelSink>();
    Logging::installSink(testSink);

    Logging::info("TestInfo");
    ASSERT_EQ(testSink->getLastMessage(), "TestInfo");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::info);

    Logging::warn("TestWarning");
    ASSERT_EQ(testSink->getLastMessage(), "TestWarning");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::warning);

    Logging::error("TestError");
    ASSERT_EQ(testSink->getLastMessage(), "TestError");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::error);

    Logging::disableLevel(Logging::Level::verbose);

    Logging::verbose("TestVerbose");
    ASSERT_EQ(testSink->getLastMessage(), "TestError");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::error);

    Logging::enableLevel(Logging::Level::verbose);

    Logging::verbose("TestVerbose");
    ASSERT_EQ(testSink->getLastMessage(), "TestVerbose");
    ASSERT_EQ(testSink->getLastLevel(), Logging::Level::verbose);

    Logging::removeSink(testSink);
}
