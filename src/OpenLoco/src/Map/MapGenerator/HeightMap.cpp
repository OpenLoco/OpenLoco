#include "HeightMap.h"
#include <algorithm>

namespace OpenLoco::World::MapGenerator
{
    constexpr uint8_t kHeightmapMarkedFlag = (1 << 7);

    void HeightMap::resetMarkerFlags()
    {
        std::for_each_n(data(), size(), [](uint8_t& value) { value &= ~kHeightmapMarkedFlag; });
    }

    uint8_t HeightMap::getHeight(Point pos) const
    {
        return (*this)[pos] & ~kHeightmapMarkedFlag;
    }

    bool HeightMap::isMarkerSet(Point pos) const
    {
        return (*this)[pos] & kHeightmapMarkedFlag;
    }

    void HeightMap::setMarker(Point pos)
    {
        (*this)[pos] |= kHeightmapMarkedFlag;
    }

    void HeightMap::unsetMarker(Point pos)
    {
        (*this)[pos] &= ~kHeightmapMarkedFlag;
    }
}
