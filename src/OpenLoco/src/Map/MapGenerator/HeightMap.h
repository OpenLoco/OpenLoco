#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenLoco::World::MapGenerator
{
    struct Point
    {
        int32_t x{};
        int32_t y{};
    };

    class HeightMap
    {
    private:
        std::vector<uint8_t> _height;

    public:
        int32_t const width;
        int32_t const height;
        int32_t const pitch;

        HeightMap(int32_t width, int32_t height, int32_t pitch)
            : _height(width * pitch)
            , width(width)
            , height(height)
            , pitch(pitch)
        {
        }

        HeightMap(const HeightMap& src)
            : _height(src._height)
            , width(src.width)
            , height(src.height)
            , pitch(src.pitch)
        {
        }

        uint8_t& operator[](Point pos)
        {
            assert(pos.x >= 0 || pos.y >= 0 || pos.x < width || pos.y < height);
            return _height[pos.y * pitch + pos.x];
        }

        const uint8_t& operator[](Point pos) const
        {
            assert(pos.x >= 0 || pos.y >= 0 || pos.x < width || pos.y < height);
            return _height[pos.y * pitch + pos.x];
        }

        uint8_t* data()
        {
            return _height.data();
        }

        const uint8_t* data() const
        {
            return _height.data();
        }

        size_t size() const
        {
            return _height.size();
        }
    };
}
