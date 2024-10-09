#include "PngImage.h"

#include <OpenLoco/Diagnostics/Logging.h>
#include <cassert>
#include <fstream>
#include <png.h>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Gfx
{
    PngImage::PngImage(int w, int h, int c)
        : width(w)
        , height(h)
        , channels(c)
    {
        assert(w > 0);
        assert(h > 0);
        assert(c > 0);

        imageData = std::vector<png_byte>(w * h * c);
    }

    Colour32 PngImage::getPixel(int x, int y)
    {
        const size_t index = (y * width + x) * channels;
        assert(index + channels < imageData.size());

        // NOTE: It always assumes the image is in RGBA format, odd PNG files will cause
        // to read garbage data.
        return {
            imageData[index + 0],
            imageData[index + 1],
            imageData[index + 2],
            imageData[index + 3]
        };
    }

    static void libpngErrorHandler(png_structp, png_const_charp error_msg)
    {
        throw std::runtime_error(error_msg);
    }

    static void libpngWarningHandler(png_structp, png_const_charp error_msg)
    {
        Logging::warn("{}", error_msg);
    }

    std::unique_ptr<PngImage> PngImage::loadFromFile(const std::filesystem::path& filePath)
    {
        std::ifstream inFile(filePath, std::ios::binary);

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, libpngErrorHandler, libpngWarningHandler);
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

        try
        {
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
        catch (const std::runtime_error& e)
        {
            Logging::error("{}", e.what());
            png_destroy_read_struct(&png, nullptr, nullptr);
            return nullptr;
        }
    }
}
