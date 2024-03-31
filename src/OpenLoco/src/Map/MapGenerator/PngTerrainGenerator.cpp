#include "PngTerrainGenerator.h"
#include "Logging.h"
#include "MapGenerator.h"
#include "PngImage.h"
#include "S5/S5.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <png.h>

using namespace OpenLoco::World;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::MapGenerator
{
    void PngTerrainGenerator::generate(const S5::Options& options, const fs::path& path, HeightMap& heightMap)
    {
        if (!fs::is_regular_file(path))
        {
            Logging::error("Can't find terrain file ({})", path);
            return;
        }

        auto pngImage = PngOps::loadPng(path.string());
        if (pngImage == nullptr)
        {
            Logging::error("Can't load terrain file ({})", path);
            return;
        }

        const int maxHeightmapLevels = 64 - options.minLandHeight;
        const float scalingFactor = maxHeightmapLevels / 255.f;

        std::fill_n(heightMap.data(), heightMap.size(), options.minLandHeight);

        auto width = std::min((int)World::kMapColumns, pngImage->width);
        auto height = std::min((int)World::kMapRows, pngImage->height);

        for (int32_t y = 0; y < height; y++)
        {
            for (int32_t x = 0; x < width; x++)
            {
                png_byte red, green, blue, alpha;
                pngImage->getPixel(x, y, red, green, blue, alpha);

                auto imgHeight = std::max({ red, green, blue });
                heightMap[{ y, x }] += imgHeight * scalingFactor; // this must be { y, x } otherwise the heightmap is mirrored
            }
        }
    }
}
