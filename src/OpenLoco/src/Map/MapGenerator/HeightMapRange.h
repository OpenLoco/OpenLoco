#pragma once

#include "HeightMap.h"

#include <cstdint>
#include <vector>

namespace OpenLoco::World::MapGenerator
{
    class HeightMapRange
    {
    private:
        HeightMap& _heightMap;
        int32_t const _minX;
        int32_t const _minY;

    public:
        int32_t const width;
        int32_t const height;

        HeightMapRange(HeightMap& heightMap)
            : _heightMap(heightMap)
            , _minX(0)
            , _minY(0)
            , width(heightMap.width)
            , height(heightMap.height)
        {
        }

        HeightMapRange(HeightMap& heightMap, int32_t minX, int32_t minY, int32_t width, int32_t height)
            : _heightMap(heightMap)
            , _minX(minX)
            , _minY(minY)
            , width(width)
            , height(height)
        {
        }

        uint8_t& operator[](Point pos)
        {
            pos.x += _minX;
            pos.y += _minY;
            return _heightMap[pos];
        }

        const uint8_t& operator[](Point pos) const
        {
            pos.x += _minX;
            pos.y += _minY;
            return _heightMap[pos];
        }

        HeightMapRange slice(int32_t l, int32_t t, int32_t w, int32_t h)
        {
            return HeightMapRange(_heightMap, _minX + l, _minY + t, w, h);
        }

        HeightMap& heightMap() { return _heightMap; }
    };
}
