#pragma once

#include "PngImage.h"
#include "Logging.h"

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::MapGenerator
{
    PngImage::PngImage(std::vector<png_byte> img, int w, int h, int c)
    {
        imageData = img;
        width = w;
        height = h;
        channels = c;
    }

    PngImage::PngImage(int w, int h, int c)
    {
        imageData = std::vector<png_byte>(w * h * c);
        width = w;
        height = h;
        channels = c;
    }

    void PngImage::getPixel(int x, int y, png_byte& red, png_byte& green, png_byte& blue, png_byte& alpha)
    {
        int index = (y * width + x) * channels;
        red = imageData[index];
        green = imageData[index + 1];
        blue = imageData[index + 2];
        alpha = imageData[index + 3];
    }

    static void my_png_error_handler(png_structp, png_const_charp error_msg)
    {
        throw std::runtime_error(error_msg);
    }

    std::unique_ptr<PngImage> PngOps::loadPng(std::string filename)
    {
        std::ifstream inFile(filename, std::ios::binary);

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, my_png_error_handler, nullptr);
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
