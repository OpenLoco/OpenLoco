#include "PngTerrainGenerator.h"
#include "Logging.h"
#include "Map/TileManager.h"
#include "MapGenerator.h"
#include "Scenario/ScenarioOptions.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Gfx/PngImage.h>
#include <OpenLoco/Platform/Platform.h>
#include <png.h>

using namespace OpenLoco::World;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::MapGenerator
{
    HeightMap PngTerrainGenerator::generate(Scenario::Options& options, const fs::path& path)
    {
        if (!fs::is_regular_file(path))
        {
            Logging::error("Can't find heightmap file ({})", path);
            return HeightMap(options.mapSizeX, options.mapSizeY);
        }

        auto pngImage = Gfx::PngImage::loadFromFile(path);
        if (pngImage == nullptr)
        {
            Logging::error("Can't load heightmap file ({})", path);
            return HeightMap(options.mapSizeX, options.mapSizeY);
        }

        // set the map to the png size
        auto width = std::max<coord_t>(World::TileManager::kMinMapDimension, std::min<coord_t>(World::TileManager::kMaxMapDimension, pngImage->width));
        auto height = std::max<coord_t>(World::TileManager::kMinMapDimension, std::min<coord_t>(World::TileManager::kMaxMapDimension, pngImage->height));

        World::TileManager::setMapSize(width, height);
        HeightMap heightMap(width, height);
        options.mapSizeX = width;
        options.mapSizeY = width;

        const int maxHeightmapLevels = 40 - options.minLandHeight;
        const float scalingFactor = maxHeightmapLevels / 255.f;

        std::fill_n(heightMap.data(), heightMap.size(), options.minLandHeight);

        for (int32_t y = 0; y < height; y++)
        {
            for (int32_t x = 0; x < width; x++)
            {
                const auto pixelColour = pngImage->getPixel(x, y);

                auto imgHeight = std::max({ pixelColour.r, pixelColour.g, pixelColour.b });
                heightMap[{ TilePos2(y, x) }] += imgHeight * scalingFactor; // this must be { y, x } otherwise the heightmap is mirrored
            }
        }

        return heightMap;
    }
}
