#include <OpenLoco/Core/Store.hpp>
#include <algorithm>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace OpenLoco;

TEST(StoreTest, defaultConstructed)
{
    Store<int> s;
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(s.size(), 0u);
    ASSERT_EQ(s.capacity(), 0u);
    ASSERT_EQ(s.begin(), s.end());
}

TEST(StoreTest, allocateAssignsSequentialIndices)
{
    Store<int> s;
    ASSERT_EQ(s.allocate(), 0u);
    ASSERT_EQ(s.allocate(), 1u);
    ASSERT_EQ(s.allocate(), 2u);
    ASSERT_EQ(s.size(), 3u);
    ASSERT_EQ(s.capacity(), 3u);
}

TEST(StoreTest, accessAndModify)
{
    Store<int> s;
    auto i = s.allocate();
    s[i] = 42;
    ASSERT_EQ(s[i], 42);

    auto j = s.allocate();
    s[j] = 99;
    ASSERT_EQ(s[i], 42);
    ASSERT_EQ(s[j], 99);
}

TEST(StoreTest, containsTracksLiveness)
{
    Store<int> s;
    ASSERT_FALSE(s.contains(0));

    auto i = s.allocate();
    ASSERT_TRUE(s.contains(i));
    ASSERT_FALSE(s.contains(999));

    s.release(i);
    ASSERT_FALSE(s.contains(i));
}

TEST(StoreTest, releaseDecreasesSize)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    auto c = s.allocate();
    ASSERT_EQ(s.size(), 3u);

    s.release(b);
    ASSERT_EQ(s.size(), 2u);
    ASSERT_TRUE(s.contains(a));
    ASSERT_FALSE(s.contains(b));
    ASSERT_TRUE(s.contains(c));
}

TEST(StoreTest, releaseDoesNotShrinkCapacity)
{
    Store<int> s;
    s.allocate();
    s.allocate();
    s.allocate();
    s.release(1);
    ASSERT_EQ(s.size(), 2u);
    ASSERT_EQ(s.capacity(), 3u);
}

TEST(StoreTest, allocateReusesFreedSlots)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    auto c = s.allocate();
    s[a] = 1;
    s[b] = 2;
    s[c] = 3;

    s.release(b);
    auto reused = s.allocate();
    ASSERT_EQ(reused, b);
    ASSERT_EQ(s.size(), 3u);
    ASSERT_EQ(s.capacity(), 3u);
}

TEST(StoreTest, allocateAfterReleaseResetsValue)
{
    Store<int> s;
    auto a = s.allocate();
    s[a] = 12345;
    s.release(a);

    auto reused = s.allocate();
    ASSERT_EQ(reused, a);
    ASSERT_EQ(s[reused], 0);
}

TEST(StoreTest, iteratorVisitsOnlyLiveSlots)
{
    Store<int> s;
    for (int v = 0; v < 5; ++v)
    {
        auto i = s.allocate();
        s[i] = v;
    }
    s.release(1);
    s.release(3);

    std::vector<int> visited;
    for (auto& v : s)
    {
        visited.push_back(v);
    }
    ASSERT_EQ(visited, (std::vector<int>{ 0, 2, 4 }));
}

TEST(StoreTest, iteratorExposesIndex)
{
    Store<int> s;
    for (int v = 0; v < 4; ++v)
    {
        auto i = s.allocate();
        s[i] = v * 10;
    }
    s.release(2);

    std::vector<std::pair<Store<int>::Index, int>> visited;
    for (auto it = s.begin(); it != s.end(); ++it)
    {
        visited.emplace_back(it.index(), *it);
    }
    ASSERT_EQ(visited.size(), 3u);
    ASSERT_EQ(visited[0], (std::pair<Store<int>::Index, int>{ 0u, 0 }));
    ASSERT_EQ(visited[1], (std::pair<Store<int>::Index, int>{ 1u, 10 }));
    ASSERT_EQ(visited[2], (std::pair<Store<int>::Index, int>{ 3u, 30 }));
}

TEST(StoreTest, iteratorSkipsLeadingAndTrailingHoles)
{
    Store<int> s;
    for (int v = 0; v < 5; ++v)
    {
        auto i = s.allocate();
        s[i] = v;
    }
    s.release(0);
    s.release(4);

    std::vector<int> visited;
    for (auto& v : s)
    {
        visited.push_back(v);
    }
    ASSERT_EQ(visited, (std::vector<int>{ 1, 2, 3 }));
}

TEST(StoreTest, iteratorMutatesValuesInPlace)
{
    Store<int> s;
    for (int v = 0; v < 3; ++v)
    {
        auto i = s.allocate();
        s[i] = v;
    }
    for (auto& v : s)
    {
        v *= 10;
    }
    ASSERT_EQ(s[0], 0);
    ASSERT_EQ(s[1], 10);
    ASSERT_EQ(s[2], 20);
}

TEST(StoreTest, constIteration)
{
    Store<int> s;
    for (int v = 0; v < 3; ++v)
    {
        auto i = s.allocate();
        s[i] = v + 1;
    }
    const auto& cs = s;

    int sum = 0;
    for (const auto& v : cs)
    {
        sum += v;
    }
    ASSERT_EQ(sum, 6);
}

TEST(StoreTest, clearResetsState)
{
    Store<int> s;
    s.allocate();
    s.allocate();
    s.allocate();
    s.release(1);

    s.clear();
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(s.size(), 0u);
    ASSERT_EQ(s.capacity(), 0u);
    ASSERT_EQ(s.begin(), s.end());

    auto fresh = s.allocate();
    ASSERT_EQ(fresh, 0u);
}

TEST(StoreTest, reserveDoesNotAffectSizeOrLiveness)
{
    Store<int> s;
    s.reserve(128);
    ASSERT_EQ(s.size(), 0u);
    ASSERT_EQ(s.capacity(), 0u);
    ASSERT_FALSE(s.contains(0));

    auto i = s.allocate();
    ASSERT_EQ(i, 0u);
    ASSERT_TRUE(s.contains(i));
}

TEST(StoreTest, growBeyondOneBitWord)
{
    Store<int> s;
    constexpr Store<int>::Index n = 200;
    for (Store<int>::Index v = 0; v < n; ++v)
    {
        auto i = s.allocate();
        s[i] = static_cast<int>(v);
    }
    ASSERT_EQ(s.size(), n);
    ASSERT_EQ(s.capacity(), n);

    for (Store<int>::Index v = 0; v < n; v += 2)
    {
        s.release(v);
    }
    ASSERT_EQ(s.size(), n / 2);

    int liveSum = 0;
    for (auto& v : s)
    {
        liveSum += v;
    }
    int expected = 0;
    for (int v = 1; v < static_cast<int>(n); v += 2)
    {
        expected += v;
    }
    ASSERT_EQ(liveSum, expected);
}

TEST(StoreTest, repeatedAllocReleaseCyclesAreStable)
{
    Store<int> s;
    std::vector<Store<int>::Index> indices;
    for (int round = 0; round < 4; ++round)
    {
        for (int i = 0; i < 16; ++i)
        {
            indices.push_back(s.allocate());
        }
        for (auto i : indices)
        {
            s.release(i);
        }
        indices.clear();
        ASSERT_EQ(s.size(), 0u);
    }
    ASSERT_EQ(s.capacity(), 16u);
}

TEST(StoreTest, worksWithNonTrivialType)
{
    Store<std::string> s;
    auto a = s.allocate();
    auto b = s.allocate();
    s[a] = "hello";
    s[b] = "world";

    ASSERT_EQ(s[a], "hello");
    ASSERT_EQ(s[b], "world");

    s.release(a);
    auto c = s.allocate();
    ASSERT_EQ(c, a);
    ASSERT_TRUE(s[c].empty());
    ASSERT_EQ(s[b], "world");
}

TEST(StoreTest, invalidIndexConstant)
{
    ASSERT_EQ(Store<int>::kInvalidIndex, std::numeric_limits<Store<int>::Index>::max());

    Store<int> s;
    auto i = s.allocate();
    ASSERT_NE(i, Store<int>::kInvalidIndex);
    ASSERT_FALSE(s.contains(Store<int>::kInvalidIndex));
}

TEST(StoreTest, allocateReturnsLowestFreeIndex)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    auto c = s.allocate();
    s[a] = 10;
    s[b] = 20;
    s[c] = 30;

    s.release(b);
    s.release(a);

    auto first = s.allocate();
    auto second = s.allocate();

    ASSERT_EQ(first, a);
    ASSERT_EQ(second, b);
}

TEST(StoreTest, mixedAllocReleaseReturnsExpectedIndices)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    auto c = s.allocate();
    auto d = s.allocate();
    ASSERT_EQ(a, 0u);
    ASSERT_EQ(b, 1u);
    ASSERT_EQ(c, 2u);
    ASSERT_EQ(d, 3u);

    s.release(b);
    s.release(d);

    auto e = s.allocate();
    ASSERT_EQ(e, b);

    auto f = s.allocate();
    ASSERT_EQ(f, d);

    auto g = s.allocate();
    ASSERT_EQ(g, 4u);
}

TEST(StoreTest, iteratorPostIncrement)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    s[a] = 7;
    s[b] = 9;

    auto it = s.begin();
    auto prev = it++;
    ASSERT_EQ(prev.index(), a);
    ASSERT_EQ(*prev, 7);
    ASSERT_EQ(it.index(), b);
    ASSERT_EQ(*it, 9);
}

TEST(StoreTest, iteratorArrowOperator)
{
    struct Point
    {
        int x;
        int y;
    };
    Store<Point> s;
    auto i = s.allocate();
    s[i].x = 3;
    s[i].y = 5;

    auto it = s.begin();
    ASSERT_EQ(it->x, 3);
    ASSERT_EQ(it->y, 5);

    it->x = 100;
    ASSERT_EQ(s[i].x, 100);
}

TEST(StoreTest, iteratorEqualityBetweenInstances)
{
    Store<int> s;
    s.allocate();
    s.allocate();
    s.allocate();

    auto a = s.begin();
    auto b = s.begin();
    ASSERT_EQ(a, b);
    ASSERT_FALSE(a != b);

    ++a;
    ASSERT_NE(a, b);
    ++b;
    ASSERT_EQ(a, b);

    auto e1 = s.end();
    auto e2 = s.end();
    ASSERT_EQ(e1, e2);
}

TEST(StoreTest, fullyReleasedStoreIteratesEmpty)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    auto c = s.allocate();
    s.release(a);
    s.release(b);
    s.release(c);

    ASSERT_EQ(s.size(), 0u);
    ASSERT_EQ(s.capacity(), 3u);
    ASSERT_EQ(s.begin(), s.end());

    int count = 0;
    for (auto& v : s)
    {
        (void)v;
        ++count;
    }
    ASSERT_EQ(count, 0);
}

TEST(StoreTest, cbeginCendExplicit)
{
    Store<int> s;
    auto a = s.allocate();
    auto b = s.allocate();
    s[a] = 4;
    s[b] = 8;

    Store<int>::const_iterator it = s.cbegin();
    int sum = 0;
    for (; it != s.cend(); ++it)
    {
        sum += *it;
    }
    ASSERT_EQ(sum, 12);
}

TEST(StoreTest, bitmapBoundarySlot63)
{
    Store<int> s;
    for (Store<int>::Index v = 0; v < 64; ++v)
    {
        auto i = s.allocate();
        s[i] = static_cast<int>(v);
    }
    ASSERT_EQ(s.capacity(), 64u);
    ASSERT_TRUE(s.contains(63));

    s.release(63);
    ASSERT_FALSE(s.contains(63));
    ASSERT_TRUE(s.contains(62));
    ASSERT_EQ(s.size(), 63u);

    auto reused = s.allocate();
    ASSERT_EQ(reused, 63u);
    ASSERT_EQ(s[reused], 0);
}

TEST(StoreTest, bitmapBoundarySlot64)
{
    Store<int> s;
    for (Store<int>::Index v = 0; v < 65; ++v)
    {
        auto i = s.allocate();
        s[i] = static_cast<int>(v);
    }
    ASSERT_EQ(s.capacity(), 65u);
    ASSERT_TRUE(s.contains(63));
    ASSERT_TRUE(s.contains(64));

    s.release(63);
    s.release(64);
    ASSERT_FALSE(s.contains(63));
    ASSERT_FALSE(s.contains(64));
    ASSERT_TRUE(s.contains(62));
    ASSERT_EQ(s.size(), 63u);
}

TEST(StoreTest, bitmapHighBitWithinWord)
{
    Store<int> s;
    for (Store<int>::Index v = 0; v < 64; ++v)
    {
        s.allocate();
    }
    for (Store<int>::Index v = 0; v < 63; ++v)
    {
        s.release(v);
    }
    ASSERT_EQ(s.size(), 1u);
    ASSERT_TRUE(s.contains(63));

    int count = 0;
    Store<int>::Index seen = Store<int>::kInvalidIndex;
    for (auto it = s.begin(); it != s.end(); ++it)
    {
        seen = it.index();
        ++count;
    }
    ASSERT_EQ(count, 1);
    ASSERT_EQ(seen, 63u);
}

TEST(StoreTest, allocateScansAcrossWordBoundary)
{
    Store<int> s;
    for (Store<int>::Index v = 0; v < 70; ++v)
    {
        s.allocate();
    }
    s.release(10);
    s.release(65);

    auto first = s.allocate();
    ASSERT_EQ(first, 10u);

    auto second = s.allocate();
    ASSERT_EQ(second, 65u);
}

TEST(StoreTest, allocateFindsHoleInLaterWordWhenEarlierWordIsFull)
{
    Store<int> s;
    for (Store<int>::Index v = 0; v < 130; ++v)
    {
        s.allocate();
    }
    s.release(100);

    auto reused = s.allocate();
    ASSERT_EQ(reused, 100u);
    ASSERT_EQ(s.capacity(), 130u);

    auto grown = s.allocate();
    ASSERT_EQ(grown, 130u);
}

TEST(StoreTest, allocateAfterClearRegrowsCleanly)
{
    Store<int> s;
    for (int v = 0; v < 130; ++v)
    {
        auto i = s.allocate();
        s[i] = v;
    }
    ASSERT_EQ(s.size(), 130u);

    s.clear();
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(s.capacity(), 0u);

    for (int v = 0; v < 130; ++v)
    {
        auto i = s.allocate();
        s[i] = v + 1000;
    }
    ASSERT_EQ(s.size(), 130u);
    ASSERT_EQ(s.capacity(), 130u);

    int liveCount = 0;
    int liveSum = 0;
    for (auto& v : s)
    {
        ++liveCount;
        liveSum += v;
    }
    ASSERT_EQ(liveCount, 130);
    int expected = 0;
    for (int v = 0; v < 130; ++v)
    {
        expected += v + 1000;
    }
    ASSERT_EQ(liveSum, expected);
}
