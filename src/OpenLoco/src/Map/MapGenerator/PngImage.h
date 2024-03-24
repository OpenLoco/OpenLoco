#pragma once

#include <memory>
#include <string>
#include <vector>

#include <png.h>

namespace OpenLoco::World::MapGenerator
{
    struct PngImage
    {
    public:
        PngImage(std::vector<png_byte> img, int w, int h, int c);

        PngImage(int w, int h, int c);

        std::vector<png_byte> imageData;
        int width;
        int height;
        int channels;

        void getPixel(int x, int y, png_byte& red, png_byte& green, png_byte& blue, png_byte& alpha);
    };

    class PngOps
    {
    public:
        static std::unique_ptr<PngImage> loadPng(std::string filename);
    };

}
