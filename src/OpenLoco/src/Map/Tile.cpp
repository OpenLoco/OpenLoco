#include "Tile.h"
#include "IndustryElement.h"
#include "Objects/ObjectManager.h"
#include "RoadElement.h"
#include "StationElement.h"
#include "SurfaceElement.h"
#include "TrackElement.h"
#include "Ui/WindowManager.h"
#include "Viewport.hpp"
#include "World/IndustryManager.h"
#include "World/Station.h"
#include <cassert>

namespace OpenLoco::World
{
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

    Tile::Tile(const TilePos2& tPos, TileElement* data)
        : _data(data)
        , pos(tPos)
    {
    }

    bool Tile::isNull() const
    {
        return _data == nullptr;
    }

    Tile::Iterator Tile::begin()
    {
        return Iterator{ _data };
    }

    Tile::Iterator Tile::begin() const
    {
        return Iterator{ _data };
    }

    Tile::Iterator Tile::end()
    {
        return Iterator{};
    }

    Tile::Iterator Tile::end() const
    {
        return Iterator{};
    }

    size_t Tile::size()
    {
        const auto dist = std::distance(begin(), end());
        return static_cast<size_t>(dist);
    }

    TileElement* Tile::operator[](size_t i)
    {
        assert(i < size());
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
        for (auto& tile : *this)
        {
            auto* result = tile.as<SurfaceElement>();
            if (result != nullptr)
            {
                return result;
            }
        }
        return nullptr;
    }

    StationElement* Tile::trainStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const
    {
        StationElement* result = nullptr;
        bool trackFound = false;
        for (auto& tile : *this)
        {
            if (trackFound)
            {
                result = tile.as<StationElement>();
                if (result != nullptr)
                {
                    break;
                }
            }
            auto* elTrack = tile.as<TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            trackFound = false;
            if (elTrack->baseZ() != baseZ)
            {
                continue;
            }
            if (elTrack->rotation() != direction)
            {
                continue;
            }
            if (elTrack->trackId() != trackId)
            {
                continue;
            }
            if (!elTrack->hasStationElement())
            {
                continue;
            }
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
                result = tile.as<StationElement>();
                if (result != nullptr)
                {
                    return result;
                }
            }
            auto* elRoad = tile.as<RoadElement>();
            if (elRoad == nullptr)
            {
                // We can have any amount of road elements between the station
                // this is different to a track where the station is always the next
                // element.
                trackFound = false;
                continue;
            }
            if (elRoad->baseZ() != baseZ)
            {
                // We can have any amount of road elements between the station
                // but if the base height is different then the station doesn't
                // exist here! (Should never happen)
                trackFound = false;
                continue;
            }
            if (elRoad->rotation() != direction)
            {
                continue;
            }
            if (elRoad->roadId() != roadId)
            {
                continue;
            }
            if (!elRoad->hasStationElement())
            {
                continue;
            }
            trackFound = true;
        }
        return result;
    }

    StationType StationElement::stationType() const { return StationType(_5 >> 5); }
    void StationElement::setStationType(StationType type)
    {
        _5 &= ~0xE0;
        _5 |= (enumValue(type) & 0x7) << 5;
    }
    /**
     *
     * @param x @<ax>
     * @param y
     * @param z
     * @param rotation
     * @return
     */
    Ui::viewport_pos gameToScreen(const Pos3& loc, int rotation)
    {
        auto rotLoc = Math::Vector::rotate(loc, rotation);
        return Ui::viewport_pos(rotLoc.y - rotLoc.x, ((rotLoc.y + rotLoc.x) >> 1) - loc.z);
    }
}
