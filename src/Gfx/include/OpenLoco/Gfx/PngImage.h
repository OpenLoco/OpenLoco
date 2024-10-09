#pragma once

#include <memory>
#include <string>
#include <vector>

#include <filesystem>
#include <png.h>

namespace OpenLoco::Gfx
{
    struct Colour32
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    class PngImage
    {
    public:
        PngImage(std::vector<png_byte> img, int w, int h, int c);

        PngImage(int w, int h, int c);

        std::vector<png_byte> imageData;
        int width{};
        int height{};
        int channels{};

        Colour32 getPixel(int x, int y);
    };

    class PngOps
    {
    public:
        static std::unique_ptr<PngImage> loadPng(const std::filesystem::path& filePath);
    };

}
