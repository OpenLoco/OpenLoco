#include "Tile.h"
#include "../IndustryManager.h"
#include "../Interop/Interop.hpp"
#include "../Objects/ObjectManager.h"
#include "../Station.h"
#include "../Ui/WindowManager.h"
#include <cassert>

using namespace OpenLoco;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::WindowManager;
using namespace OpenLoco::Map;

const uint8_t* tile_element_base::data() const
{
    return (uint8_t*)this;
}

element_type tile_element_base::type() const
{
    return (element_type)((_type & 0x3C) >> 2);
}

bool tile_element_base::isLast() const
{
    return (_flags & ElementFlags::last) != 0;
}

BuildingObject* building_element::object() const
{
    return ObjectManager::get<BuildingObject>(objectId());
}

tile::tile(tile_coord_t x, tile_coord_t y, tile_element* data)
    : _data(data)
    , x(x)
    , y(y)
{
}

bool tile::isNull() const
{
    return _data == nullptr;
}

tile_element* tile::begin()
{
    return _data;
}

tile_element* tile::begin() const
{
    return const_cast<tile&>(*this).begin();
}

tile_element* tile::end()
{
    auto el = _data;
    do
    {
        el++;
    } while (!(el - 1)->isLast());
    return el;
}

tile_element* tile::end() const
{
    return const_cast<tile&>(*this).end();
}

size_t tile::size()
{
    return end() - begin();
}

tile_element* tile::operator[](size_t i)
{
#if DEBUG
    assert(i < size());
#endif
    return &_data[i];
}

size_t tile::indexOf(const tile_element_base* element) const
{
    size_t i = 0;
    for (const auto& tile : *this)
    {
        if (&tile == element)
        {
            return i;
        }
        i++;
    }
    return npos;
}

surface_element* tile::surface() const
{
    surface_element* result = nullptr;
    for (auto& tile : *this)
    {
        result = tile.asSurface();
        if (result != nullptr)
        {
            break;
        }
    }
    return result;
}

station_element* tile::trackStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const
{
    station_element* result = nullptr;
    bool trackFound = false;
    for (auto& tile : *this)
    {
        if (trackFound)
        {
            result = tile.asStation();
            if (result != nullptr)
            {
                break;
            }
        }
        auto* elTrack = tile.asTrack();
        if (elTrack == nullptr)
            continue;
        trackFound = false;
        if (elTrack->baseZ() != baseZ)
            continue;
        if (elTrack->unkDirection() != direction)
            continue;
        if (elTrack->trackId() != trackId)
            continue;
        if (!elTrack->hasStationElement())
            continue;
        trackFound = true;
    }
    return result;
}

station_element* tile::roadStation(uint8_t roadId, uint8_t direction, uint8_t baseZ) const
{
    station_element* result = nullptr;
    bool trackFound = false;
    for (auto& tile : *this)
    {
        if (trackFound)
        {
            result = tile.asStation();
            if (result != nullptr)
            {
                break;
            }
        }
        auto* elRoad = tile.asRoad();
        if (elRoad == nullptr)
            continue;
        trackFound = false;
        if (elRoad->baseZ() != baseZ)
            continue;
        if (elRoad->unkDirection() != direction)
            continue;
        if (elRoad->roadId() != roadId)
            continue;
        if (!elRoad->hasStationElement())
            continue;
        trackFound = true;
    }
    return result;
}

OpenLoco::Industry* industry_element::industry() const
{
    return IndustryManager::get(_industryId);
}

OpenLoco::StationType station_element::stationType() const { return OpenLoco::StationType(_5 >> 5); }

namespace OpenLoco::Map
{
    /**
     *
     * @param x @<ax>
     * @param y
     * @param z
     * @param rotation
     * @return
     */
    Ui::viewport_pos coordinate3dTo2d(int16_t x, int16_t y, int16_t z, int rotation)
    {
        Ui::viewport_pos coordinate_2d;

        switch (rotation)
        {
            default:
            case 0:
                coordinate_2d.x = y - x;
                coordinate_2d.y = ((y + x) >> 1) - z;
                break;
            case 1:
                coordinate_2d.x = -y - x;
                coordinate_2d.y = ((y - x) >> 1) - z;
                break;
            case 2:
                coordinate_2d.x = -y + x;
                coordinate_2d.y = ((-y - x) >> 1) - z;
                break;
            case 3:
                coordinate_2d.x = y + x;
                coordinate_2d.y = ((-y + x) >> 1) - z;
                break;
        }

        return coordinate_2d;
    }

    Pos2 rotate2dCoordinate(Pos2 pos, uint8_t rotation)
    {
        Pos2 coordinate2D;

        switch (rotation)
        {
            default:
            case 0:
                coordinate2D = pos;
                break;
            case 1:
                coordinate2D.x = pos.y;
                coordinate2D.y = -pos.x;
                break;
            case 2:
                coordinate2D.x = -pos.x;
                coordinate2D.y = -pos.y;
                break;
            case 3:
                coordinate2D.x = -pos.y;
                coordinate2D.y = pos.x;
                break;
        }

        return coordinate2D;
    }
}
