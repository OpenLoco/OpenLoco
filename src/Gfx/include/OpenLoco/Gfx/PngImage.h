#pragma once

#include <OpenLoco/Core/FileSystem.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

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
        std::vector<uint8_t> imageData;

    public:
        const int width{};
        const int height{};
        const int channels{};

        PngImage() = default;
        PngImage(int w, int h, int c);

        static std::unique_ptr<PngImage> loadFromFile(const fs::path& filePath);

        Colour32 getPixel(int x, int y);
    };

}
