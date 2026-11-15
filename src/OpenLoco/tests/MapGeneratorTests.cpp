#include <OpenLoco/Gfx/PngImage.h>
#include <OpenLoco/Map/MapGenerator/PngTerrainGenerator.h>
#include <OpenLoco/Map/TileManager.h>
#include <gtest/gtest.h>

using namespace OpenLoco;
using namespace OpenLoco::World;

namespace
{
    // The size a map has before the heightmap is imported
    constexpr auto kDefaultSize = TilePos2{ 384, 384 };

    TilePos2 getMapSizeForImageSize(const int32_t width, const int32_t height)
    {
        const Gfx::PngImage image{ width, height, 3 };
        return MapGenerator::PngTerrainGenerator::getMapSizeForImage(image, kDefaultSize);
    }

    TEST(MapGeneratorTest, heightmapImagesUseTheirNativeSizeWhenItIsAValidMapSize)
    {
        // An image that is itself a valid map size is used at its native resolution
        EXPECT_EQ(getMapSizeForImageSize(16, 16), (TilePos2{ 16, 16 }));
        EXPECT_EQ(getMapSizeForImageSize(1024, 512), (TilePos2{ 1024, 512 }));
        EXPECT_EQ(getMapSizeForImageSize(4096, 4096), (TilePos2{ 4096, 4096 }));
    }

    TEST(MapGeneratorTest, heightmapImagesAreScaledWhenTheirSizeIsNotAValidMapSize)
    {
        // Images that are too small or too large are scaled to the size the map already has
        EXPECT_EQ(getMapSizeForImageSize(TileManager::kMinMapDimension - 1, 512), kDefaultSize);
        EXPECT_EQ(getMapSizeForImageSize(512, TileManager::kMinMapDimension - 1), kDefaultSize);
        EXPECT_EQ(getMapSizeForImageSize(TileManager::kMaxMapDimension + 1, 512), kDefaultSize);
        EXPECT_EQ(getMapSizeForImageSize(512, TileManager::kMaxMapDimension + 1), kDefaultSize);
    }
}
