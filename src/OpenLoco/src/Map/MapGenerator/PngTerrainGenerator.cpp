#include "PngTerrainGenerator.h"
#include "Logging.h"
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

        // TODO: Move the constants to a more sensible place, values are taken from TileManager::adjustSurfaceHeight
        constexpr int minLandHeight = 4 / kMicroToSmallZStep;
        constexpr int maxLandHeight = 160 / kMicroToSmallZStep;

        const int heightRange = maxLandHeight - std::max<int>(minLandHeight, options.minLandHeight);

        std::fill_n(heightMap.data(), heightMap.size(), options.minLandHeight);

        // Map the entire map area to the image with interpolation
        for (int32_t y = 0; y < World::kMapRows; y++)
        {
            for (int32_t x = 0; x < World::kMapColumns; x++)
            {
                // Map from map coordinates to image coordinates
                const float imgX = (x * pngImage->width) / static_cast<float>(World::kMapColumns);
                const float imgY = (y * pngImage->height) / static_cast<float>(World::kMapRows);

                // Bilinear interpolation coordinates
                const int x0 = static_cast<int>(imgX);
                const int y0 = static_cast<int>(imgY);
                const int x1 = std::min(x0 + 1, pngImage->width - 1);
                const int y1 = std::min(y0 + 1, pngImage->height - 1);

                const float fx = imgX - x0;
                const float fy = imgY - y0;

                // Sample the four corners for bilinear interpolation
                const auto c00 = pngImage->getPixel(x0, y0);
                const auto c10 = pngImage->getPixel(x1, y0);
                const auto c01 = pngImage->getPixel(x0, y1);
                const auto c11 = pngImage->getPixel(x1, y1);

                // Get max of RGB channels for each corner
                const auto h00 = std::max({ c00.r, c00.g, c00.b });
                const auto h10 = std::max({ c10.r, c10.g, c10.b });
                const auto h01 = std::max({ c01.r, c01.g, c01.b });
                const auto h11 = std::max({ c11.r, c11.g, c11.b });

                // Perform bilinear interpolation
                const auto h0 = h00 * (1.0f - fx) + h10 * fx;
                const auto h1 = h01 * (1.0f - fx) + h11 * fx;
                const auto imgHeight = h0 * (1.0f - fy) + h1 * fy;

                // Map pixel brightness (0-255) to terrain height (minLandHeight to maxLandHeight)
                const auto normalizedHeight = imgHeight / 255.0f;
                heightMap[{ TilePos2(y, x) }] = options.minLandHeight + (normalizedHeight * heightRange); // this must be { y, x } otherwise the heightmap is mirrored
            }
        }
    }
}
