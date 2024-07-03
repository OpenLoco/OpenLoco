#include "HeightMap.h"
#include <algorithm>

namespace OpenLoco::World::MapGenerator
{
    constexpr uint8_t kHeightmapMarkedFlag = (1 << 7);

    void HeightMap::resetMarkerFlags()
    {
        std::for_each_n(data(), size(), [](uint8_t& value) { value &= ~kHeightmapMarkedFlag; });
    }

    uint8_t HeightMap::getHeight(TilePos2 pos) const
    {
        return (*this)[pos] & ~kHeightmapMarkedFlag;
    }

    bool HeightMap::isMarkerSet(TilePos2 pos) const
    {
        return (*this)[pos] & kHeightmapMarkedFlag;
    }

    void HeightMap::setMarker(TilePos2 pos)
    {
        (*this)[pos] |= kHeightmapMarkedFlag;
    }

    void HeightMap::unsetMarker(TilePos2 pos)
    {
        (*this)[pos] &= ~kHeightmapMarkedFlag;
    }
}
