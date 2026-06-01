#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Map/RoadElement.h>
#include <OpenLoco/Map/StationElement.h>
#include <OpenLoco/Map/SurfaceElement.h>
#include <OpenLoco/Map/Tile.h>
#include <OpenLoco/Map/TileElement.h>
#include <OpenLoco/Map/TileManager.h>
#include <OpenLoco/Map/TrackElement.h>
#include <OpenLoco/Map/TreeElement.h>
#include <OpenLoco/World/Station.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

using namespace OpenLoco::World;

namespace
{
    class TileManagerTest : public ::testing::Test
    {
    protected:
        static void SetUpTestSuite()
        {
            TileManager::allocateMapElements();
        }

        void SetUp() override
        {
            TileManager::initialise();
        }

        static ElementType typeAt(Tile tile, size_t index)
        {
            size_t n = 0;
            for (auto& el : tile)
            {
                if (n++ == index)
                {
                    return el.type();
                }
            }
            ADD_FAILURE() << "index " << index << " out of range";
            return ElementType::surface;
        }

        using TileBytes = std::vector<std::array<uint8_t, kTileElementSize>>;

        static TileBytes snapshotBytes(Tile tile)
        {
            TileBytes out;
            for (size_t i = 0; i < tile.size(); ++i)
            {
                std::array<uint8_t, kTileElementSize> bytes{};
                std::memcpy(bytes.data(), tile[i], kTileElementSize);
                out.push_back(bytes);
            }
            return out;
        }
    };

    constexpr TilePos2 kTestTile{ 10, 5 };
    constexpr TilePos2 kOtherTile{ 11, 5 };
}

TEST_F(TileManagerTest, InitialiseLeavesOneSurfacePerTile)
{
    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), 1u);
    ASSERT_NE(tile.surface(), nullptr);
    EXPECT_EQ(tile.surface()->baseZ(), 4);
    EXPECT_TRUE(tile.begin()->isLast());
}

TEST_F(TileManagerTest, InsertSingleElementSetsTypeBaseZAndIsLast)
{
    auto* inserted = TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(inserted, nullptr);
    EXPECT_EQ(inserted->type(), ElementType::track);
    EXPECT_EQ(inserted->baseZ(), 8);
    EXPECT_TRUE(inserted->isLast());

    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), 2u);
}

TEST_F(TileManagerTest, MultipleInsertsAreOrderedByBaseZ)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 16, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 24, 0);

    auto tile = TileManager::get(kTestTile);
    std::vector<uint8_t> baseZs;
    std::vector<ElementType> types;
    int lastCount = 0;
    for (auto& el : tile)
    {
        baseZs.push_back(el.baseZ());
        types.push_back(el.type());
        if (el.isLast())
        {
            ++lastCount;
        }
    }
    ASSERT_EQ(baseZs.size(), 4u);
    EXPECT_TRUE(std::ranges::is_sorted(baseZs));
    EXPECT_EQ(types.front(), ElementType::surface);
    EXPECT_EQ(lastCount, 1);
}

TEST_F(TileManagerTest, RemoveMiddleElementShiftsRemainder)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 24, 0);

    TileElementEntry* middle = nullptr;
    for (auto& el : TileManager::get(kTestTile))
    {
        if (el.type() == ElementType::tree)
        {
            middle = &el;
            break;
        }
    }
    ASSERT_NE(middle, nullptr);

    const auto freeBefore = TileManager::numFreeElements();
    TileManager::removeElement(*middle);
    EXPECT_EQ(TileManager::numFreeElements(), freeBefore + 1);

    auto tile = TileManager::get(kTestTile);
    std::vector<ElementType> types;
    int lastCount = 0;
    for (auto& el : tile)
    {
        types.push_back(el.type());
        if (el.isLast())
        {
            ++lastCount;
        }
    }
    ASSERT_EQ(types.size(), 3u);
    EXPECT_EQ(types[0], ElementType::surface);
    EXPECT_EQ(types[1], ElementType::track);
    EXPECT_EQ(types[2], ElementType::track);
    EXPECT_EQ(lastCount, 1);
}

TEST_F(TileManagerTest, RemoveLastElementTransfersIsLastFlag)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    auto* last = TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 24, 0);
    ASSERT_NE(last, nullptr);
    ASSERT_TRUE(last->isLast());

    TileManager::setRemoveElementPointerChecker(*last);
    TileManager::removeElement(*last);
    EXPECT_TRUE(TileManager::wasRemoveOnLastElement());

    auto tile = TileManager::get(kTestTile);
    std::vector<uint8_t> baseZs;
    int lastCount = 0;
    uint8_t lastBaseZ = 0;
    for (auto& el : tile)
    {
        baseZs.push_back(el.baseZ());
        if (el.isLast())
        {
            ++lastCount;
            lastBaseZ = el.baseZ();
        }
    }
    ASSERT_EQ(baseZs.size(), 2u);
    EXPECT_EQ(lastCount, 1);
    EXPECT_EQ(lastBaseZ, 8);
}

TEST_F(TileManagerTest, TilesAreIsolatedFromEachOther)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kOtherTile), 16, 0);

    auto a = TileManager::get(kTestTile);
    auto b = TileManager::get(kOtherTile);

    std::vector<ElementType> aTypes;
    for (auto& el : a)
    {
        aTypes.push_back(el.type());
    }
    std::vector<ElementType> bTypes;
    for (auto& el : b)
    {
        bTypes.push_back(el.type());
    }

    ASSERT_EQ(aTypes.size(), 2u);
    ASSERT_EQ(bTypes.size(), 2u);
    EXPECT_EQ(aTypes[0], ElementType::surface);
    EXPECT_EQ(aTypes[1], ElementType::track);
    EXPECT_EQ(bTypes[0], ElementType::surface);
    EXPECT_EQ(bTypes[1], ElementType::tree);
}

TEST_F(TileManagerTest, NumFreeElementsDecreasesMonotonicallyOnInsert)
{
    const auto initial = TileManager::numFreeElements();
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    const auto afterFirst = TileManager::numFreeElements();
    EXPECT_LT(afterFirst, initial);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    const auto afterSecond = TileManager::numFreeElements();
    EXPECT_LT(afterSecond, afterFirst);
}

TEST_F(TileManagerTest, InsertElementPropagatesOccupiedQuads)
{
    auto* inserted = TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0b1010);
    ASSERT_NE(inserted, nullptr);
    EXPECT_EQ(inserted->occupiedQuarter(), 0b1010);
}

TEST_F(TileManagerTest, GetOutOfBoundsReturnsNullTile)
{
    auto t = TileManager::get(TilePos2(kMapColumns + 100, kMapRows + 100));
    EXPECT_TRUE(t.isNull());
    EXPECT_EQ(t.begin(), t.end());
}

TEST_F(TileManagerTest, GetByPos2AndCoordsAgreeWithTilePos2)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    auto a = TileManager::get(kTestTile);
    auto b = TileManager::get(toWorldSpace(kTestTile));
    auto c = TileManager::get(toWorldSpace(kTestTile).x, toWorldSpace(kTestTile).y);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(c.size(), 2u);
    EXPECT_EQ(&*a.begin(), &*b.begin());
    EXPECT_EQ(&*a.begin(), &*c.begin());
}

TEST_F(TileManagerTest, TileSizeMatchesIteration)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    auto tile = TileManager::get(kTestTile);
    size_t walked = 0;
    for ([[maybe_unused]] auto& el : tile)
    {
        ++walked;
    }
    EXPECT_EQ(tile.size(), walked);
    EXPECT_EQ(tile.size(), 3u);
}

TEST_F(TileManagerTest, TileSubscriptMatchesIterationOrder)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    auto tile = TileManager::get(kTestTile);

    std::vector<std::pair<ElementType, uint8_t>> walked;
    for (auto& el : tile)
    {
        walked.emplace_back(el.type(), el.baseZ());
    }
    ASSERT_EQ(walked.size(), tile.size());
    for (size_t i = 0; i < tile.size(); ++i)
    {
        EXPECT_EQ(typeAt(tile, i), walked[i].first);
        EXPECT_EQ(tile[i]->baseZ(), walked[i].second);
    }
}

TEST_F(TileManagerTest, TileIndexOfFindsMemberAndMissesNonMember)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), 3u);
    EXPECT_EQ(tile.indexOf(tile[0]), 0u);
    EXPECT_EQ(tile.indexOf(tile[1]), 1u);
    EXPECT_EQ(tile.indexOf(tile[2]), 2u);

    auto otherTile = TileManager::get(kOtherTile);
    EXPECT_EQ(tile.indexOf(otherTile[0]), Tile::npos);
}

TEST_F(TileManagerTest, BoundaryTilesAreUsable)
{
    constexpr TilePos2 origin{ 0, 0 };
    constexpr TilePos2 corner{ kMapColumns - 1, kMapRows - 1 };
    TileManager::insertElement(ElementType::track, toWorldSpace(origin), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(corner), 16, 0);

    auto a = TileManager::get(origin);
    auto b = TileManager::get(corner);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(typeAt(a, 1), ElementType::track);
    EXPECT_EQ(typeAt(b, 1), ElementType::tree);
}

TEST_F(TileManagerTest, InsertElementAfterNoReorgPlacesRightAfterTarget)
{
    auto* anchor = TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(anchor, nullptr);

    auto* inserted = TileManager::insertElementAfterNoReorg(
        anchor, ElementType::tree, toWorldSpace(kTestTile), 4, 0);
    ASSERT_NE(inserted, nullptr);

    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), 3u);

    bool sawAnchor = false;
    bool nextIsTree = false;
    for (auto& el : tile)
    {
        if (sawAnchor)
        {
            nextIsTree = (el.type() == ElementType::tree);
            break;
        }
        if (el.type() == ElementType::track && el.baseZ() == 8)
        {
            sawAnchor = true;
        }
    }
    EXPECT_TRUE(sawAnchor);
    EXPECT_TRUE(nextIsTree);
}

TEST_F(TileManagerTest, CheckFreeElementsAndReorganiseSucceedsWithDefaultSpace)
{
    EXPECT_TRUE(TileManager::checkFreeElementsAndReorganise());
}

TEST_F(TileManagerTest, ReorganisePreservesEveryTouchedTile)
{
    struct Entry
    {
        TilePos2 tile;
        ElementType type;
        uint8_t baseZ;
    };
    const std::vector<Entry> entries{
        { TilePos2{ 5, 5 }, ElementType::track, 8 },
        { TilePos2{ 5, 5 }, ElementType::tree, 16 },
        { TilePos2{ 5, 5 }, ElementType::track, 24 },
        { TilePos2{ 5, 6 }, ElementType::tree, 12 },
        { TilePos2{ 6, 5 }, ElementType::track, 4 },
        { TilePos2{ 6, 5 }, ElementType::tree, 32 },
        { TilePos2{ 50, 50 }, ElementType::track, 8 },
        { TilePos2{ kMapColumns - 1, kMapRows - 1 }, ElementType::tree, 8 },
    };
    for (const auto& e : entries)
    {
        TileManager::insertElement(e.type, toWorldSpace(e.tile), e.baseZ, 0);
    }

    auto snapshot = [&](const Entry& e) {
        auto tile = TileManager::get(e.tile);
        std::vector<std::pair<ElementType, uint8_t>> v;
        for (auto& el : tile)
        {
            v.emplace_back(el.type(), el.baseZ());
        }
        return v;
    };

    std::vector<std::vector<std::pair<ElementType, uint8_t>>> before;
    for (const auto& e : entries)
    {
        before.push_back(snapshot(e));
    }

    TileManager::reorganise();

    for (size_t i = 0; i < entries.size(); ++i)
    {
        EXPECT_EQ(snapshot(entries[i]), before[i]);
    }
}

TEST_F(TileManagerTest, DefragmentTilePeriodicCompactsTilePointerBackward)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);

    auto tileBefore = TileManager::get(kTestTile);
    ASSERT_EQ(tileBefore.size(), 2u);
    auto* entryAddrBefore = &*tileBefore.begin();

    constexpr size_t kNumTiles = static_cast<size_t>(kMapPitch) * static_cast<size_t>(kMapRows);
    for (size_t i = 0; i < kNumTiles + 1; ++i)
    {
        TileManager::defragmentTilePeriodic();
    }

    auto tileAfter = TileManager::get(kTestTile);
    ASSERT_EQ(tileAfter.size(), 2u);
    auto* entryAddrAfter = &*tileAfter.begin();
    EXPECT_LT(entryAddrAfter, entryAddrBefore);
    EXPECT_EQ(typeAt(tileAfter, 0), ElementType::surface);
    EXPECT_EQ(typeAt(tileAfter, 1), ElementType::track);
    EXPECT_EQ(tileAfter[1]->baseZ(), 8);
}

TEST_F(TileManagerTest, DefragmentTilePeriodicReducesElementsEnd)
{
    for (int i = 0; i < 100; ++i)
    {
        TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), static_cast<uint8_t>(8 + i), 0);
    }
    const auto liveBefore = TileManager::getEntries().size();

    constexpr size_t kNumTiles = static_cast<size_t>(kMapPitch) * static_cast<size_t>(kMapRows);
    for (size_t i = 0; i < kNumTiles + 1; ++i)
    {
        TileManager::defragmentTilePeriodic();
    }

    EXPECT_LT(TileManager::getEntries().size(), liveBefore);
}

TEST_F(TileManagerTest, DefragmentTilePeriodicPreservesByteContentAfterChurn)
{
    const std::vector<TilePos2> tiles{
        TilePos2{ 5, 5 }, TilePos2{ 5, 6 }, TilePos2{ 6, 5 }, TilePos2{ 100, 100 }, TilePos2{ 200, 200 }
    };
    for (auto t : tiles)
    {
        TileManager::insertElement(ElementType::track, toWorldSpace(t), 8, 0);
        TileManager::insertElement(ElementType::tree, toWorldSpace(t), 16, 0);
    }
    for (auto t : tiles)
    {
        TileElementEntry* victim = nullptr;
        for (auto& el : TileManager::get(t))
        {
            if (el.type() == ElementType::tree)
            {
                victim = &el;
                break;
            }
        }
        ASSERT_NE(victim, nullptr);
        TileManager::removeElement(*victim);
    }

    std::vector<TileBytes> before;
    for (auto t : tiles)
    {
        before.push_back(snapshotBytes(TileManager::get(t)));
    }

    constexpr size_t kNumTiles = static_cast<size_t>(kMapPitch) * static_cast<size_t>(kMapRows);
    for (size_t i = 0; i < 2 * kNumTiles; ++i)
    {
        TileManager::defragmentTilePeriodic();
    }

    for (size_t i = 0; i < tiles.size(); ++i)
    {
        EXPECT_EQ(before[i], snapshotBytes(TileManager::get(tiles[i])))
            << "tile (" << tiles[i].x << "," << tiles[i].y << ") diverged after periodic defrag";
    }
}

TEST_F(TileManagerTest, DisablePeriodicDefragSkipsNextCall)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);

    constexpr size_t kNumTiles = static_cast<size_t>(kMapPitch) * static_cast<size_t>(kMapRows);
    for (size_t i = 0; i < kNumTiles + 1; ++i)
    {
        TileManager::defragmentTilePeriodic();
    }

    TileManager::insertElement(ElementType::tree, toWorldSpace(kOtherTile), 8, 0);
    const auto liveBeforeDisabled = TileManager::getEntries().size();

    TileManager::disablePeriodicDefrag();
    TileManager::defragmentTilePeriodic();
    EXPECT_EQ(TileManager::getEntries().size(), liveBeforeDisabled);
}

TEST_F(TileManagerTest, DefragmentTilePeriodicIsSafeAfterChurn)
{
    const std::vector<TilePos2> tiles{
        { 5, 5 }, { 5, 6 }, { 6, 5 }, { 100, 100 }, { 200, 200 }
    };
    for (auto t : tiles)
    {
        TileManager::insertElement(ElementType::track, toWorldSpace(t), 8, 0);
        TileManager::insertElement(ElementType::tree, toWorldSpace(t), 16, 0);
    }
    for (auto t : tiles)
    {
        TileElementEntry* toRemove = nullptr;
        for (auto& el : TileManager::get(t))
        {
            if (el.type() == ElementType::tree)
            {
                toRemove = &el;
                break;
            }
        }
        ASSERT_NE(toRemove, nullptr);
        TileManager::removeElement(*toRemove);
    }

    for (int i = 0; i < 4096; ++i)
    {
        TileManager::defragmentTilePeriodic();
    }

    for (auto t : tiles)
    {
        auto tile = TileManager::get(t);
        size_t n = 0;
        int lastCount = 0;
        for (auto& el : tile)
        {
            ++n;
            if (el.isLast())
            {
                ++lastCount;
            }
        }
        EXPECT_EQ(n, 2u);
        EXPECT_EQ(lastCount, 1);
    }
}

TEST_F(TileManagerTest, GetEntriesReflectsLiveRegion)
{
    auto entries = TileManager::getEntries();
    EXPECT_FALSE(entries.empty());
    for (auto& entry : entries)
    {
        EXPECT_EQ(entry.type(), ElementType::surface);
        EXPECT_TRUE(entry.isLast());
    }
}

TEST_F(TileManagerTest, GetEntriesCountGrowsAfterInsert)
{
    const auto before = TileManager::getEntries().size();
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    EXPECT_GT(TileManager::getEntries().size(), before);
}

TEST_F(TileManagerTest, UpdateTilePointersRebuildsConsistently)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kOtherTile), 12, 0);
    TileManager::reorganise();
    TileManager::updateTilePointers();

    auto a = TileManager::get(kTestTile);
    auto b = TileManager::get(kOtherTile);
    ASSERT_EQ(a.size(), 2u);
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(typeAt(a, 0), ElementType::surface);
    EXPECT_EQ(typeAt(a, 1), ElementType::track);
    EXPECT_EQ(typeAt(b, 0), ElementType::surface);
    EXPECT_EQ(typeAt(b, 1), ElementType::tree);
}

TEST_F(TileManagerTest, BaseZTiesPreserveInsertionOrder)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 8, 0);

    auto tile = TileManager::get(kTestTile);
    std::vector<ElementType> types;
    for (auto& el : tile)
    {
        types.push_back(el.type());
    }
    ASSERT_EQ(types.size(), 3u);
    EXPECT_EQ(types[0], ElementType::surface);
    EXPECT_EQ(types[1], ElementType::track);
    EXPECT_EQ(types[2], ElementType::tree);
}

TEST_F(TileManagerTest, IteratorDefaultConstructedEqualsEnd)
{
    auto tile = TileManager::get(kTestTile);
    Tile::Iterator def{};
    EXPECT_EQ(def, tile.end());
}

TEST_F(TileManagerTest, IteratorPostIncrementReturnsOldPosition)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    auto tile = TileManager::get(kTestTile);
    auto it = tile.begin();
    auto pre = it;
    auto post = it++;
    EXPECT_EQ(pre, post);
    EXPECT_NE(it, post);
    EXPECT_EQ(post->type(), ElementType::surface);
    EXPECT_EQ(it->type(), ElementType::track);
}

TEST_F(TileManagerTest, ManyElementsOnOneTileAllWalk)
{
    constexpr int kCount = static_cast<int>(TileManager::kMaxElementsOnOneTile) - 1;
    for (int i = 0; i < kCount; ++i)
    {
        const auto baseZ = static_cast<uint8_t>(std::min(8 + i, 255));
        const auto type = (i % 2 == 0) ? ElementType::track : ElementType::tree;
        ASSERT_NE(TileManager::insertElement(type, toWorldSpace(kTestTile), baseZ, 0), nullptr);
    }

    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), static_cast<size_t>(kCount + 1));

    int lastCount = 0;
    uint8_t prevZ = 0;
    int idx = 0;
    for (auto& el : tile)
    {
        EXPECT_GE(el.baseZ(), prevZ);
        prevZ = el.baseZ();
        if (el.isLast())
        {
            ++lastCount;
        }
        if (idx == 0)
        {
            EXPECT_EQ(el.type(), ElementType::surface);
        }
        ++idx;
    }
    EXPECT_EQ(lastCount, 1);
}

TEST_F(TileManagerTest, ReorganiseIsIdempotent)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(kOtherTile), 4, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(TilePos2{ 50, 50 }), 24, 0);

    TileManager::reorganise();
    const auto a1 = snapshotBytes(TileManager::get(kTestTile));
    const auto b1 = snapshotBytes(TileManager::get(kOtherTile));
    const auto c1 = snapshotBytes(TileManager::get(TilePos2{ 50, 50 }));
    const auto freeAfter1 = TileManager::numFreeElements();

    TileManager::reorganise();
    const auto a2 = snapshotBytes(TileManager::get(kTestTile));
    const auto b2 = snapshotBytes(TileManager::get(kOtherTile));
    const auto c2 = snapshotBytes(TileManager::get(TilePos2{ 50, 50 }));

    EXPECT_EQ(a1, a2);
    EXPECT_EQ(b1, b2);
    EXPECT_EQ(c1, c2);
    EXPECT_EQ(TileManager::numFreeElements(), freeAfter1);
}

TEST_F(TileManagerTest, ReorganisePreservesRawElementBytes)
{
    auto* el1 = TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0b0101);
    ASSERT_NE(el1, nullptr);
    el1->setGhost(true);
    auto* el2 = TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0b1010);
    ASSERT_NE(el2, nullptr);
    el2->setAiAllocated(true);
    el2->setFlag6(true);

    const auto before = snapshotBytes(TileManager::get(kTestTile));
    TileManager::reorganise();
    const auto after = snapshotBytes(TileManager::get(kTestTile));

    ASSERT_EQ(before.size(), after.size());
    for (size_t i = 0; i < before.size(); ++i)
    {
        EXPECT_EQ(before[i], after[i]) << "byte mismatch at element " << i;
    }
}

TEST_F(TileManagerTest, RemoveDoesNotChangeOtherTileBytes)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kOtherTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kOtherTile), 16, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(TilePos2{ 100, 100 }), 12, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(TilePos2{ 100, 100 }), 20, 0);

    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 24, 0);

    const auto otherBefore = snapshotBytes(TileManager::get(kOtherTile));
    const auto farBefore = snapshotBytes(TileManager::get(TilePos2{ 100, 100 }));

    TileElementEntry* victim = nullptr;
    for (auto& el : TileManager::get(kTestTile))
    {
        if (el.type() == ElementType::tree)
        {
            victim = &el;
            break;
        }
    }
    ASSERT_NE(victim, nullptr);
    TileManager::removeElement(*victim);

    EXPECT_EQ(otherBefore, snapshotBytes(TileManager::get(kOtherTile)));
    EXPECT_EQ(farBefore, snapshotBytes(TileManager::get(TilePos2{ 100, 100 })));
}

TEST_F(TileManagerTest, WasRemoveOnLastElementFalseForMiddleRemoval)
{
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    TileManager::insertElement(ElementType::tree, toWorldSpace(kTestTile), 16, 0);
    TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 24, 0);

    TileElementEntry* middle = nullptr;
    for (auto& el : TileManager::get(kTestTile))
    {
        if (el.type() == ElementType::tree)
        {
            middle = &el;
            break;
        }
    }
    ASSERT_NE(middle, nullptr);

    TileManager::setRemoveElementPointerChecker(*middle);
    TileManager::removeElement(*middle);
    EXPECT_FALSE(TileManager::wasRemoveOnLastElement());
}

TEST_F(TileManagerTest, InsertElementRoadBasicAddsRoadElement)
{
    auto* road = TileManager::insertElementRoad(toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(road, nullptr);
    EXPECT_EQ(road->type(), ElementType::road);
    EXPECT_EQ(road->baseZ(), 8);

    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), 2u);
    EXPECT_EQ(typeAt(tile, 0), ElementType::surface);
    EXPECT_EQ(typeAt(tile, 1), ElementType::road);
}

TEST_F(TileManagerTest, InsertElementRoadStopsBeforeRoadStationAtSameBaseZ)
{
    auto* stationEntry = TileManager::insertElement<StationElement>(toWorldSpace(kTestTile), 16, 0);
    ASSERT_NE(stationEntry, nullptr);
    stationEntry->get<StationElement>().setStationType(OpenLoco::StationType::roadStation);

    auto* road = TileManager::insertElementRoad(toWorldSpace(kTestTile), 16, 0);
    ASSERT_NE(road, nullptr);

    auto tile = TileManager::get(kTestTile);
    ASSERT_EQ(tile.size(), 3u);
    EXPECT_EQ(typeAt(tile, 0), ElementType::surface);
    EXPECT_EQ(typeAt(tile, 1), ElementType::road);
    EXPECT_EQ(typeAt(tile, 2), ElementType::station);
}

TEST_F(TileManagerTest, InsertElementTypedTemplateReturnsTypedPointer)
{
    auto* track = TileManager::insertElement<TrackElement>(toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(track, nullptr);
    EXPECT_EQ(track->type(), ElementType::track);
    EXPECT_EQ(track->baseZ(), 8);

    auto* tree = TileManager::insertElement<TreeElement>(toWorldSpace(kTestTile), 12, 0);
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->type(), ElementType::tree);
}

TEST_F(TileManagerTest, InsertElementAfterNoReorgTypedTemplateReturnsTypedPointer)
{
    auto* anchor = TileManager::insertElement(ElementType::track, toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(anchor, nullptr);
    auto* station = TileManager::insertElementAfterNoReorg<StationElement>(anchor, toWorldSpace(kTestTile), 4, 0);
    ASSERT_NE(station, nullptr);
    EXPECT_EQ(station->type(), ElementType::station);
}

TEST_F(TileManagerTest, TileSurfaceReturnsTheSurfaceElement)
{
    auto tile = TileManager::get(kTestTile);
    auto* surface = tile.surface();
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(typeAt(tile, 0), ElementType::surface);
    EXPECT_EQ(surface->baseZ(), 4);
    EXPECT_EQ(static_cast<TileElement*>(surface), tile[0]);
}

TEST_F(TileManagerTest, TileTrainStationFindsStationFollowingMatchingTrack)
{
    auto* trackEntry = TileManager::insertElement<TrackElement>(toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(trackEntry, nullptr);
    auto& track = trackEntry->get<TrackElement>();
    track.setRotation(2);
    track.setTrackId(5);
    track.setHasStationElement(true);

    TileElementEntry* anchor = nullptr;
    for (auto& el : TileManager::get(kTestTile))
    {
        if (el.type() == ElementType::track)
        {
            anchor = &el;
            break;
        }
    }
    ASSERT_NE(anchor, nullptr);
    auto* stationEntry = TileManager::insertElementAfterNoReorg<StationElement>(anchor, toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(stationEntry, nullptr);
    auto* station = stationEntry->as<StationElement>();

    auto tile = TileManager::get(kTestTile);
    auto* found = tile.trainStation(5, 2, 8);
    EXPECT_EQ(found, station);

    EXPECT_EQ(tile.trainStation(6, 2, 8), nullptr);
    EXPECT_EQ(tile.trainStation(5, 3, 8), nullptr);
    EXPECT_EQ(tile.trainStation(5, 2, 12), nullptr);
}

TEST_F(TileManagerTest, TileRoadStationFindsStationFollowingMatchingRoad)
{
    auto* roadEntry = TileManager::insertElementRoad(toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(roadEntry, nullptr);
    auto& road = roadEntry->get<RoadElement>();
    road.setRotation(1);
    road.setRoadId(3);
    road.setHasStationElement(true);

    TileElementEntry* anchor = nullptr;
    for (auto& el : TileManager::get(kTestTile))
    {
        if (el.type() == ElementType::road)
        {
            anchor = &el;
            break;
        }
    }
    ASSERT_NE(anchor, nullptr);
    auto* stationEntry = TileManager::insertElementAfterNoReorg<StationElement>(anchor, toWorldSpace(kTestTile), 8, 0);
    ASSERT_NE(stationEntry, nullptr);
    auto* station = stationEntry->as<StationElement>();

    auto tile = TileManager::get(kTestTile);
    auto* found = tile.roadStation(3, 1, 8);
    EXPECT_EQ(found, station);

    EXPECT_EQ(tile.roadStation(4, 1, 8), nullptr);
}

TEST_F(TileManagerTest, ManyInsertsTriggerReorganiseInternallyWithoutBreakingTile)
{
    constexpr int kInserts = 20000;
    for (int i = 0; i < kInserts; ++i)
    {
        const auto baseZ = static_cast<uint8_t>(8 + (i % 200));
        const TilePos2 t{ static_cast<int16_t>(i % 64), static_cast<int16_t>((i / 64) % 64) };
        ASSERT_NE(TileManager::insertElement(ElementType::track, toWorldSpace(t), baseZ, 0), nullptr);
    }

    for (int row = 0; row < 64; ++row)
    {
        for (int col = 0; col < 64; ++col)
        {
            auto tile = TileManager::get(TilePos2{ static_cast<int16_t>(col), static_cast<int16_t>(row) });
            ASSERT_GE(tile.size(), 1u);
            int lastCount = 0;
            uint8_t prev = 0;
            for (auto& el : tile)
            {
                EXPECT_GE(el.baseZ(), prev);
                prev = el.baseZ();
                if (el.isLast())
                {
                    ++lastCount;
                }
            }
            EXPECT_EQ(lastCount, 1);
        }
    }
}

TEST_F(TileManagerTest, ReorganiseReclaimsFragmentationGarbage)
{
    const auto freeAfterInit = TileManager::numFreeElements();
    for (int i = 0; i < 2000; ++i)
    {
        const TilePos2 t{ static_cast<int16_t>(i % 64), static_cast<int16_t>((i / 64) % 64) };
        ASSERT_NE(TileManager::insertElement(ElementType::track, toWorldSpace(t), static_cast<uint8_t>(8 + (i % 200)), 0), nullptr);
    }
    const auto freeAfterChurn = TileManager::numFreeElements();
    EXPECT_LT(freeAfterChurn, freeAfterInit);

    TileManager::reorganise();
    const auto freeAfterReorg = TileManager::numFreeElements();
    EXPECT_GT(freeAfterReorg, freeAfterChurn);
}

TEST_F(TileManagerTest, ReorganiseAfterChurnMatchesByteShadow)
{
    struct Step
    {
        enum class Kind
        {
            Insert,
            Remove,
        };
        Kind kind;
        TilePos2 tile;
        ElementType type{ ElementType::surface };
        uint8_t baseZ{ 0 };
    };

    const std::vector<Step> steps{
        { Step::Kind::Insert, TilePos2{ 5, 5 }, ElementType::track, 8 },
        { Step::Kind::Insert, TilePos2{ 5, 5 }, ElementType::tree, 16 },
        { Step::Kind::Insert, TilePos2{ 5, 6 }, ElementType::track, 12 },
        { Step::Kind::Insert, TilePos2{ 6, 5 }, ElementType::tree, 24 },
        { Step::Kind::Insert, TilePos2{ 5, 5 }, ElementType::track, 32 },
        { Step::Kind::Remove, TilePos2{ 5, 5 }, ElementType::tree, 16 },
        { Step::Kind::Insert, TilePos2{ 6, 5 }, ElementType::track, 8 },
        { Step::Kind::Remove, TilePos2{ 6, 5 }, ElementType::tree, 24 },
        { Step::Kind::Insert, TilePos2{ 50, 50 }, ElementType::track, 8 },
    };

    for (const auto& s : steps)
    {
        if (s.kind == Step::Kind::Insert)
        {
            ASSERT_NE(TileManager::insertElement(s.type, toWorldSpace(s.tile), s.baseZ, 0), nullptr);
        }
        else
        {
            TileElementEntry* victim = nullptr;
            for (auto& el : TileManager::get(s.tile))
            {
                if (el.type() == s.type && el.baseZ() == s.baseZ)
                {
                    victim = &el;
                    break;
                }
            }
            ASSERT_NE(victim, nullptr);
            TileManager::removeElement(*victim);
        }
    }

    const std::vector<TilePos2> tilesOfInterest{
        TilePos2{ 5, 5 }, TilePos2{ 5, 6 }, TilePos2{ 6, 5 }, TilePos2{ 50, 50 }
    };
    std::vector<TileBytes> before;
    for (auto t : tilesOfInterest)
    {
        before.push_back(snapshotBytes(TileManager::get(t)));
    }

    TileManager::reorganise();

    for (size_t i = 0; i < tilesOfInterest.size(); ++i)
    {
        EXPECT_EQ(before[i], snapshotBytes(TileManager::get(tilesOfInterest[i])))
            << "tile (" << tilesOfInterest[i].x << "," << tilesOfInterest[i].y << ") diverged after reorganise";
    }
}
