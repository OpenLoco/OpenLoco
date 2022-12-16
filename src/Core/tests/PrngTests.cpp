#include <OpenLoco/Core/Prng.h>
#include <gtest/gtest.h>

using namespace OpenLoco;

TEST(PrngTests, misc)
{
    Core::Prng prng;
    EXPECT_EQ(prng.randNext(), 0);
    EXPECT_EQ(prng.srand_0(), 0xFE2468AC);
    EXPECT_EQ(prng.srand_1(), 0);

    prng = Core::Prng(0x1234, 0x4321);
    EXPECT_EQ(prng.srand_0(), 0x1234);
    EXPECT_EQ(prng.srand_1(), 0x4321);
    EXPECT_EQ(prng.randNext(), 0x80000246);
    EXPECT_EQ(prng.srand_0(), 0xBC247A5E);
    EXPECT_EQ(prng.srand_1(), 0x80000246);

    for (auto i = 0; i < 1000; ++i)
    {
        prng.randNext();
    }
    EXPECT_EQ(prng.srand_0(), 0x0A597A43);
    EXPECT_EQ(prng.srand_1(), 0x12FC0827);

    {
        constexpr size_t kMaxVal = 30;
        EXPECT_EQ(prng.randNext(kMaxVal), 11);
        for (auto i = 0; i < 1000; ++i)
        {
            EXPECT_LE(prng.randNext(kMaxVal), kMaxVal);
        }

        constexpr size_t kMinVal = 10;
        EXPECT_EQ(prng.randNext(kMinVal, kMaxVal), 26);
        for (auto i = 0; i < 1000; ++i)
        {
            const auto randVal = prng.randNext(kMinVal, kMaxVal);
            EXPECT_LE(randVal, kMaxVal);
            EXPECT_GE(randVal, kMinVal);
        }
    }
    {
        constexpr int32_t kMaxVal = -1;
        constexpr size_t kMinVal = -10;
        EXPECT_EQ(prng.randNext(kMinVal, kMaxVal), -4);
        for (auto i = 0; i < 1000; ++i)
        {
            const auto randVal = prng.randNext(kMinVal, kMaxVal);
            EXPECT_LE(randVal, kMaxVal);
            EXPECT_GE(randVal, kMinVal);
        }
    }
    // These tests fail. Never call randNext with back to front min/max
    //{
    //    constexpr int32_t kMaxVal = -30;
    //    EXPECT_EQ(prng.randNext(kMaxVal), 12);
    //    for (auto i = 0; i < 1000; ++i)
    //    {
    //        EXPECT_LE(prng.randNext(kMaxVal), kMaxVal);
    //    }

    //    constexpr int32_t kMinVal = 10;
    //    EXPECT_EQ(prng.randNext(kMinVal, kMaxVal), 12);
    //    for (auto i = 0; i < 1000; ++i)
    //    {
    //        const auto randVal = prng.randNext(10, kMaxVal);
    //        EXPECT_LT(randVal, kMaxVal);
    //        EXPECT_GE(randVal, kMinVal);
    //    }
    //}
    {
        // Negatives for single var randNext wrap around
        constexpr int32_t kMaxVal = -1;
        EXPECT_EQ(prng.randNext(kMaxVal), 0x78E536FB);
        for (auto i = 0; i < 1000; ++i)
        {
            EXPECT_LE(prng.randNext(kMaxVal), kMaxVal & 0x7FFFFFFF);
        }
    }
    EXPECT_EQ(prng.randNext(0), 0);
    EXPECT_EQ(prng.randNext(0, 0), 0);
}
