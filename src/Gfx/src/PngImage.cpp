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
        assert(index + channels <= imageData.size());

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

            // Apply transformations to normalize to RGBA format
            png_byte colorType = png_get_color_type(png, info);
            png_byte bitDepth = png_get_bit_depth(png, info);

            // Convert palette to RGB
            if (colorType == PNG_COLOR_TYPE_PALETTE)
            {
                png_set_palette_to_rgb(png);
            }

            // Convert grayscale to RGB if less than 8 bits
            if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
            {
                png_set_expand_gray_1_2_4_to_8(png);
            }

            // Add alpha channel if not present
            if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_PALETTE)
            {
                png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
            }

            // Convert 16-bit to 8-bit
            if (bitDepth == 16)
            {
                png_set_strip_16(png);
            }

            // Convert grayscale to RGB
            if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
            {
                png_set_gray_to_rgb(png);
            }

            // Update info after transformations
            png_read_update_info(png, info);

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
