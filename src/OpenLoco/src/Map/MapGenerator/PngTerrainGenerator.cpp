#include "PngTerrainGenerator.h"
#include "Logging.h"
#include "MapGenerator.h"
#include "S5/S5.h"
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
        PngImage(std::vector<png_byte> img, int w, int h, int c)
        {
            imageData = img;
            width = w;
            height = h;
            channels = c;
        }

        PngImage(int w, int h, int c)
        {
            imageData = std::vector<png_byte>(w * h * c);
            width = w;
            height = h;
            channels = c;
        }

        std::vector<png_byte> imageData;
        int width;
        int height;
        int channels;

        void getPixel(int x, int y, png_byte& red, png_byte& green, png_byte& blue, png_byte& alpha)
        {
            int index = (y * width + x) * channels;
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
            int channels = png_get_channels(png, info);

            auto pngImage = std::make_unique<PngImage>(width, height, channels);

            png_bytep* rowPointers = new png_bytep[height];
            for (int y = 0; y < height; y++)
            {
                rowPointers[y] = &pngImage->imageData[y * width * channels];
            }
            png_read_image(png, rowPointers);

            // cleanup image
            delete[] rowPointers;
            png_destroy_read_struct(&png, &info, nullptr);
            inFile.close();

            return pngImage;
        }
    };

    void PngTerrainGenerator::generate(const S5::Options& options, const fs::path& path, HeightMapRange heightMap)
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

        const auto width = std::min<int16_t>(World::kMapColumns, pngImage->width);
        const auto height = std::min<int16_t>(World::kMapRows, pngImage->height);

        for (int32_t y = 0; y < height; y++)
        {
            for (int32_t x = 0; x < width; x++)
            {
                png_byte red, green, blue, alpha;
                pngImage->getPixel(x, y, red, green, blue, alpha);

                auto imgHeight = (red + green + blue) / 3;
                heightMap[{ x, y }] = options.minLandHeight + (imgHeight * scalingFactor);
            }
        }
    }
}
