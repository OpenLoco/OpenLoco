#include "PngTerrainGenerator.h"
#include "Logging.h"
#include "MapGenerator.h"
#include "ScenarioOptions.h"
#include <Map/TileManager.h>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Gfx/PngImage.h>
#include <OpenLoco/Platform/Platform.h>
#include <png.h>

using namespace OpenLoco::World;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::MapGenerator
{
    void PngTerrainGenerator::generate(const Scenario::Options& options, const fs::path& path, HeightMap& heightMap)
    {
        if (!fs::is_regular_file(path))
        {
            Logging::error("Can't find heightmap file ({})", path);
            return;
        }

        auto pngImage = Gfx::PngImage::loadFromFile(path);
        if (pngImage == nullptr)
        {
            Logging::error("Can't load heightmap file ({})", path);
            return;
        }

        const int maxHeightmapLevels = 40 - options.minLandHeight;
        const float scalingFactor = maxHeightmapLevels / 255.f;

        std::fill_n(heightMap.data(), heightMap.size(), options.minLandHeight);

        auto width = std::min<int>(World::TileManager::getMapColumns(), pngImage->width);
        auto height = std::min<int>(World::TileManager::getMapRows(), pngImage->height);

        for (int32_t y = 0; y < height; y++)
        {
            for (int32_t x = 0; x < width; x++)
            {
                const auto pixelColour = pngImage->getPixel(x, y);

                auto imgHeight = std::max({ pixelColour.r, pixelColour.g, pixelColour.b });
                heightMap[{ TilePos2(y, x) }] += imgHeight * scalingFactor; // this must be { y, x } otherwise the heightmap is mirrored
            }
        }
    }
}
