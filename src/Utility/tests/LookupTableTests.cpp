#include <OpenLoco/Utility/LookupTable.hpp>
#include <gtest/gtest.h>

using namespace OpenLoco;

TEST(LookupTableTests, buildLookupTable)
{
    constexpr auto table = Utility::buildLookupTable<int, int>({
        { 3, 30 },
        { 1, 10 },
        { 2, 20 },
    });

    EXPECT_EQ(table.size(), 3);
}

TEST(LookupTableTests, sortedOrder)
{
    constexpr auto table = Utility::buildLookupTable<int, int>({
        { 3, 30 },
        { 1, 10 },
        { 2, 20 },
    });

    auto it = table.begin();
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 3);
}

TEST(LookupTableTests, find)
{
    constexpr auto table = Utility::buildLookupTable<int, int>({
        { 5, 50 },
        { 3, 30 },
        { 1, 10 },
    });

    auto it = table.find(3);
    ASSERT_NE(it, table.end());
    EXPECT_EQ(it->second, 30);

    EXPECT_EQ(table.find(99), table.end());
}

TEST(LookupTableTests, contains)
{
    constexpr auto table = Utility::buildLookupTable<int, int>({
        { 1, 10 },
        { 2, 20 },
    });

    EXPECT_TRUE(table.contains(1));
    EXPECT_TRUE(table.contains(2));
    EXPECT_FALSE(table.contains(3));
}

TEST(LookupTableTests, at)
{
    constexpr auto table = Utility::buildLookupTable<int, int>({
        { 1, 10 },
        { 2, 20 },
    });

    EXPECT_EQ(table.at(1), 10);
    EXPECT_EQ(table.at(2), 20);
    EXPECT_THROW(table.at(99), std::out_of_range);
}

TEST(LookupTableTests, constexprEvaluation)
{
    constexpr auto table = Utility::buildLookupTable<int, int>({
        { 2, 20 },
        { 1, 10 },
    });

    static_assert(table.size() == 2);
    static_assert(table.contains(1));
    static_assert(table.contains(2));
    static_assert(!table.contains(3));
    static_assert(table.at(1) == 10);
    static_assert(table.at(2) == 20);
}
