#include <OpenLoco/GameState.h>
#include <OpenLoco/S5/S5Types.h>
#include <OpenLoco/S5/S5WidePositions.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <gtest/gtest.h>
#include <limits>
#include <utility>

using namespace OpenLoco;
using namespace OpenLoco::S5;

namespace
{
    constexpr coord_t kLargeCoord = 120000;
    constexpr coord_t kSmallCoord = 500;

    class WidePositionsTest : public ::testing::Test
    {
    protected:
        GameState& state = getGameState();
    };

    TEST_F(WidePositionsTest, coordFitsInSaveFormat)
    {
        EXPECT_TRUE(coordFitsInSaveFormat(0));
        EXPECT_TRUE(coordFitsInSaveFormat(std::numeric_limits<int16_t>::max()));
        EXPECT_TRUE(coordFitsInSaveFormat(std::numeric_limits<int16_t>::min()));
        EXPECT_FALSE(coordFitsInSaveFormat(static_cast<coord_t>(std::numeric_limits<int16_t>::max()) + 1));
        EXPECT_FALSE(coordFitsInSaveFormat(static_cast<coord_t>(std::numeric_limits<int16_t>::min()) - 1));

        // A 1024 tile map spans coordinates up to 32736, which is the largest map that fits entirely
        EXPECT_TRUE(coordFitsInSaveFormat((1024 - 1) * World::kTileSize));
        EXPECT_FALSE(coordFitsInSaveFormat(1024 * World::kTileSize));
    }

    TEST_F(WidePositionsTest, encodeDecodeRoundTrip)
    {
        const std::vector<WidePosition> positions{
            WidePosition{ 0, kLargeCoord },
            WidePosition{ 17, -kLargeCoord },
            WidePosition{ 1000, 32768 },
        };

        const auto decoded = decodeWidePositions(encodeWidePositions(positions));
        ASSERT_EQ(decoded.size(), positions.size());
        EXPECT_EQ(decoded[0], positions[0]);
        EXPECT_EQ(decoded[1], positions[1]);
        EXPECT_EQ(decoded[2], positions[2]);
    }

    TEST_F(WidePositionsTest, encodeDecodeEmpty)
    {
        // Maps that do not need any of this still write the chunk, so it must be a valid empty chunk
        const auto data = encodeWidePositions({});
        EXPECT_EQ(data.size(), sizeof(uint32_t) * 2);
        EXPECT_TRUE(decodeWidePositions(data).empty());
    }

    TEST_F(WidePositionsTest, decodeIgnoresNewerVersion)
    {
        auto data = encodeWidePositions({ WidePosition{ 0, kLargeCoord } });

        uint32_t newerVersion = kCurrentWidePositionVersion + 1;
        std::memcpy(data.data(), &newerVersion, sizeof(newerVersion));

        EXPECT_TRUE(decodeWidePositions(data).empty());
    }

    TEST_F(WidePositionsTest, decodeRejectsInvalidData)
    {
        EXPECT_THROW(decodeWidePositions({}), std::exception);

        auto data = encodeWidePositions({ WidePosition{ 0, kLargeCoord } });
        data.pop_back();
        EXPECT_THROW(decodeWidePositions(data), std::exception);
    }

    TEST_F(WidePositionsTest, noPositionsForSmallCoordinates)
    {
        // A map that only uses coordinates that fit in the save format should not need any of this
        EXPECT_TRUE(collectWidePositions(state).empty());
    }

    TEST_F(WidePositionsTest, coordinatesSurviveSaveRoundTrip)
    {
        // Save the fields this test uses so that they can be restored afterwards
        const auto oldSavedView = std::pair{ state.savedViewX, state.savedViewY };
        const auto oldTown = std::pair{ state.towns[1].x, state.towns[1].y };
        const auto oldEntityPos = state.entities[3].position;
        const auto oldStation = std::pair{ state.stations[2].x, state.stations[2].stationTiles[0].x };
        auto& hashEntry = state.companies[1].trackandRoadHashTable[7];
        const auto oldHashEntry = hashEntry;

        // Coordinates that the vanilla save format cannot store
        state.savedViewX = kLargeCoord;
        state.savedViewY = kLargeCoord + World::kTileSize;
        state.towns[1].x = kLargeCoord;
        state.towns[1].y = -kLargeCoord;
        state.entities[3].position = World::Pos3{ -kLargeCoord, kLargeCoord, 64 };
        state.stations[2].x = kLargeCoord;
        state.stations[2].stationTiles[0].x = -kLargeCoord;
        hashEntry.posX = kLargeCoord;
        hashEntry.posY = -kLargeCoord;

        const auto widePositions = collectWidePositions(state);
        ASSERT_FALSE(widePositions.empty());

        // Emulate the way the vanilla save format clamps what it cannot store
        state.savedViewX = toSaveCoord(state.savedViewX);
        state.savedViewY = toSaveCoord(state.savedViewY);
        state.towns[1].x = toSaveCoord(state.towns[1].x);
        state.towns[1].y = toSaveCoord(state.towns[1].y);
        state.entities[3].position.x = toSaveCoord(state.entities[3].position.x);
        state.stations[2].x = toSaveCoord(state.stations[2].x);
        state.stations[2].stationTiles[0].x = toSaveCoord(state.stations[2].stationTiles[0].x);
        hashEntry.posX = toSaveCoord(hashEntry.posX);
        hashEntry.posY = toSaveCoord(hashEntry.posY);

        // Loading a save file applies the stored coordinates on top of the clamped ones
        applyWidePositions(state, decodeWidePositions(encodeWidePositions(widePositions)));

        EXPECT_EQ(state.savedViewX, kLargeCoord);
        EXPECT_EQ(state.savedViewY, kLargeCoord + World::kTileSize);
        EXPECT_EQ(state.towns[1].x, kLargeCoord);
        EXPECT_EQ(state.towns[1].y, -kLargeCoord);
        EXPECT_EQ(state.entities[3].position.x, -kLargeCoord);
        EXPECT_EQ(state.entities[3].position.y, kLargeCoord);
        EXPECT_EQ(state.stations[2].x, kLargeCoord);
        EXPECT_EQ(state.stations[2].stationTiles[0].x, -kLargeCoord);
        EXPECT_EQ(hashEntry.posX, kLargeCoord);
        EXPECT_EQ(hashEntry.posY, -kLargeCoord);

        // Restore the original state
        state.savedViewX = oldSavedView.first;
        state.savedViewY = oldSavedView.second;
        state.towns[1].x = oldTown.first;
        state.towns[1].y = oldTown.second;
        state.entities[3].position = oldEntityPos;
        state.stations[2].x = oldStation.first;
        state.stations[2].stationTiles[0].x = oldStation.second;
        hashEntry = oldHashEntry;
    }
}
