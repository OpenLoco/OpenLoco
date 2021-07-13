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

const uint8_t* TileElementBase::data() const
{
    return (uint8_t*)this;
}

ElementType TileElementBase::type() const
{
    return (ElementType)((_type & 0x3C) >> 2);
}

bool TileElementBase::isLast() const
{
    return (_flags & ElementFlags::last) != 0;
}

BuildingObject* BuildingElement::object() const
{
    return ObjectManager::get<BuildingObject>(objectId());
}

Tile::Tile(const TilePos2& tPos, TileElement* data)
    : _data(data)
    , pos(tPos)
{
}

bool Tile::isNull() const
{
    return _data == nullptr;
}

TileElement* Tile::begin()
{
    return _data;
}

TileElement* Tile::begin() const
{
    return const_cast<Tile&>(*this).begin();
}

TileElement* Tile::end()
{
    auto el = _data;
    do
    {
        el++;
    } while (!(el - 1)->isLast());
    return el;
}

TileElement* Tile::end() const
{
    return const_cast<Tile&>(*this).end();
}

size_t Tile::size()
{
    return end() - begin();
}

TileElement* Tile::operator[](size_t i)
{
#if DEBUG
    assert(i < size());
#endif
    return &_data[i];
}

size_t Tile::indexOf(const TileElementBase* element) const
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

SurfaceElement* Tile::surface() const
{
    SurfaceElement* result = nullptr;
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

StationElement* Tile::trackStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const
{
    StationElement* result = nullptr;
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

StationElement* Tile::roadStation(uint8_t roadId, uint8_t direction, uint8_t baseZ) const
{
    StationElement* result = nullptr;
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

OpenLoco::Industry* IndustryElement::industry() const
{
    return IndustryManager::get(_industryId);
}

OpenLoco::StationType StationElement::stationType() const { return OpenLoco::StationType(_5 >> 5); }

uint8_t IndustryElement::var_6_1F() const
{
    return (_6 >> 6) & 0x1F;
}

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
}
