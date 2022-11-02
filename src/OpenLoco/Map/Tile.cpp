#include "Tile.h"
#include "../IndustryManager.h"
#include "../Objects/ObjectManager.h"
#include "../Station.h"
#include "../Ui/WindowManager.h"
#include "../Viewport.hpp"
#include "IndustryElement.h"
#include "RoadElement.h"
#include "StationElement.h"
#include "SurfaceElement.h"
#include "TrackElement.h"
#include <cassert>

namespace OpenLoco::Map
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
        for (auto& tile : *this)
        {
            auto* result = tile.as<SurfaceElement>();
            if (result != nullptr)
                return result;
        }
        return nullptr;
    }

    StationElement* Tile::trackStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const
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
                result = tile.as<StationElement>();
                if (result != nullptr)
                {
                    break;
                }
            }
            auto* elRoad = tile.as<RoadElement>();
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

    StationType StationElement::stationType() const { return StationType(_5 >> 5); }

    Industry* IndustryElement::industry() const
    {
        return IndustryManager::get(_industryId);
    }

    uint8_t IndustryElement::buildingType() const
    {
        return (_6 >> 6) & 0x1F;
    }

    Colour IndustryElement::var_6_F800() const
    {
        return static_cast<Colour>((_6 >> 11) & 0x1F);
    }

    uint8_t IndustryElement::var_6_003F() const
    {
        return _6 & 0x3F;
    }

    uint8_t IndustryElement::sectionProgress() const
    {
        return (_5 >> 5) & 0x7;
    }

    uint8_t IndustryElement::sequenceIndex() const
    {
        return _5 & 0x3;
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
