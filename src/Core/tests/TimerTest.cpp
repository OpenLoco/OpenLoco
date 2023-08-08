#include <OpenLoco/Core/Timer.hpp>
#include <gtest/gtest.h>
#include <thread>

using namespace OpenLoco;

TEST(TimerTest, TestDuration)
{
    Core::Timer timer;

    // Should be close to zero at this point.
    EXPECT_NEAR(timer.elapsed(), 0.0f, 0.001f);

    // Should be close to 1000ms after 1 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Need a somewhat high tolerance here, as sleeping is not very accurate.
    EXPECT_NEAR(timer.elapsed(), 1000.0f, 20.0f);
}

TEST(TimerTest, TestReset)
{
    Core::Timer timer;

    // Should be close to 1000ms after 1 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timer.reset();

    // Should be close to zero at this point.
    EXPECT_NEAR(timer.elapsed(), 0.0f, 0.001f);
}
