#include "PngTerrainGenerator.h"
#include "Logging.h"
#include "MapGenerator.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <png.h>

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non-portable

using namespace OpenLoco::World;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::MapGenerator
{
    struct PngImage
    {
    public:
        PngImage(std::vector<png_byte> img, int w, int h)
        {
            imageData = img;
            width = w;
            height = h;
        }

        PngImage(int w, int h)
        {
            imageData = std::vector<png_byte>(w * h * 4);
            width = w;
            height = h;
        }

        std::vector<png_byte> imageData;
        int width;
        int height;

        void getPixel(int x, int y, png_byte& red, png_byte& green, png_byte& blue, png_byte& alpha)
        {
            int index = (y * width + x) * 4;
            red = imageData[index];
            green = imageData[index + 1];
            blue = imageData[index + 2];
            alpha = imageData[index + 3];
        }
    };

    class PngOps
    {
    public:
        static std::unique_ptr<PngImage> loadPng(std::string filename)
        {
            std::ifstream inFile(filename, std::ios::binary);

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png)
            {
                Logging::error("Failed to create PNG read struct");
                return nullptr;
            }

            png_infop info = png_create_info_struct(png);
            if (!info)
            {
                Logging::error("Failed to create PNG info struct");
                png_destroy_read_struct(&png, nullptr, nullptr);
                return nullptr;
            }

            // Set error handler
            if (setjmp(png_jmpbuf(png)))
            {
                png_destroy_read_struct(&png, &info, nullptr);
                Logging::error("Failed to set png error handler");
                return nullptr;
            }

            png_set_read_fn(png, static_cast<void*>(&inFile), [](png_structp png_ptr, png_bytep data, png_size_t length) {
                std::istream* inStream = static_cast<std::istream*>(png_get_io_ptr(png_ptr));
                inStream->read(reinterpret_cast<char*>(data), length);
            });

            png_read_info(png, info);

            int width = png_get_image_width(png, info);
            int height = png_get_image_height(png, info);
            // png_byte colorType = png_get_color_type(png, info);

            auto pngImage = std::make_unique<PngImage>(width, height);

            png_bytep* rowPointers = new png_bytep[height];
            for (int y = 0; y < height; y++)
            {
                rowPointers[y] = &pngImage->imageData[y * width * 4];
            }
            png_read_image(png, rowPointers);

            // cleanup image
            delete[] rowPointers;
            png_destroy_read_struct(&png, &info, nullptr);
            inFile.close();

            return pngImage;
        }

        /*
        static void savePng()
        {
        }
        */
    };

    void PngTerrainGenerator::generate(const fs::path& path, HeightMapRange heightMap)
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

        constexpr int maxHeightmapLevels = 40;
        constexpr float ScalingFactor = maxHeightmapLevels / 255.f; // scaling factor from rgb to loco-height
        auto width = std::min<int16_t>(World::kMapColumns, pngImage->width);
        auto height = std::min<int16_t>(World::kMapRows, pngImage->height);

        for (int32_t y = 0; y < height; y++)
        {
            for (int32_t x = 0; x < width; x++)
            {
                png_byte red, green, blue, alpha;
                pngImage->getPixel(x, y, red, green, blue, alpha);
                auto imgHeight = ((red + green + blue) / 3); // [0, 255]

                // flipped x,y is intentional here since openloco is flipping them somewhere else down the line
                heightMap[{ y, x }] = (int)(imgHeight * ScalingFactor) + 1; // [1, maxHeightmapLevels];
            }
        }
    }
}
