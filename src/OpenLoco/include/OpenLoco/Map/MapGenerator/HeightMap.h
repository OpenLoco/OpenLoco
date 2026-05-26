#pragma once

#include <OpenLoco/Engine/World.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenLoco::World::MapGenerator
{
    class HeightMap
    {
    private:
        std::vector<uint8_t> _height;

    public:
        const uint16_t width;
        const uint16_t height;

        HeightMap(int32_t width, int32_t height)
            : _height(width * height)
            , width(width)
            , height(height)
        {
        }

        uint8_t& operator[](TilePos2 pos)
        {
            assert(pos.x >= 0 || pos.y >= 0 || pos.x < width || pos.y < height);
            return _height[pos.y * width + pos.x];
        }

        const uint8_t& operator[](TilePos2 pos) const
        {
            assert(pos.x >= 0 || pos.y >= 0 || pos.x < width || pos.y < height);
            return _height[pos.y * width + pos.x];
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

        uint8_t getHeight(TilePos2 pos) const;
        void resetMarkerFlags();
        bool isMarkerSet(TilePos2 pos) const;
        void setMarker(TilePos2 pos);
        void unsetMarker(TilePos2 pos);
    };
}
