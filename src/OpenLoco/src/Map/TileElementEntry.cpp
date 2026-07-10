#include "Map/TileElementEntry.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"

namespace OpenLoco::World
{
    TileElement& TileElementEntry::operator*() const
    {
        return TileManager::resolveEntry(this);
    }

    TileElement* TileElementEntry::operator->() const
    {
        return &TileManager::resolveEntry(this);
    }

    uint8_t TileElementEntry::flags() const
    {
        return TileManager::resolveEntry(this).flags();
    }

    SmallZ TileElementEntry::baseZ() const
    {
        return TileManager::resolveEntry(this).baseZ();
    }

    void TileElementEntry::setBaseZ(SmallZ v)
    {
        TileManager::resolveEntry(this).setBaseZ(v);
    }

    int16_t TileElementEntry::baseHeight() const
    {
        return TileManager::resolveEntry(this).baseHeight();
    }

    SmallZ TileElementEntry::clearZ() const
    {
        return TileManager::resolveEntry(this).clearZ();
    }

    void TileElementEntry::setClearZ(SmallZ v)
    {
        TileManager::resolveEntry(this).setClearZ(v);
    }

    int16_t TileElementEntry::clearHeight() const
    {
        return TileManager::resolveEntry(this).clearHeight();
    }

    bool TileElementEntry::isGhost() const
    {
        return TileManager::resolveEntry(this).isGhost();
    }

    void TileElementEntry::setGhost(bool state)
    {
        TileManager::resolveEntry(this).setGhost(state);
    }

    bool TileElementEntry::isAiAllocated() const
    {
        return TileManager::resolveEntry(this).isAiAllocated();
    }

    void TileElementEntry::setAiAllocated(bool state)
    {
        TileManager::resolveEntry(this).setAiAllocated(state);
    }

    bool TileElementEntry::isFlag6() const
    {
        return TileManager::resolveEntry(this).isFlag6();
    }

    void TileElementEntry::setFlag6(bool state)
    {
        TileManager::resolveEntry(this).setFlag6(state);
    }

    uint8_t TileElementEntry::occupiedQuarter() const
    {
        return TileManager::resolveEntry(this).occupiedQuarter();
    }

    void TileElementEntry::setOccupiedQuarter(uint8_t v)
    {
        TileManager::resolveEntry(this).setOccupiedQuarter(v);
    }

    const uint8_t* TileElementEntry::data() const
    {
        return TileManager::resolveEntry(this).data();
    }

    std::span<uint8_t> TileElementEntry::rawData() const
    {
        return TileManager::resolveEntry(this).rawData();
    }

    template<typename T>
    T& TileElementEntry::get() const
    {
        assert(type() == T::kElementType);
        return TileManager::getStore<T>()[index()];
    }

    template SurfaceElement& TileElementEntry::get<SurfaceElement>() const;
    template TrackElement& TileElementEntry::get<TrackElement>() const;
    template StationElement& TileElementEntry::get<StationElement>() const;
    template SignalElement& TileElementEntry::get<SignalElement>() const;
    template BuildingElement& TileElementEntry::get<BuildingElement>() const;
    template TreeElement& TileElementEntry::get<TreeElement>() const;
    template WallElement& TileElementEntry::get<WallElement>() const;
    template RoadElement& TileElementEntry::get<RoadElement>() const;
    template IndustryElement& TileElementEntry::get<IndustryElement>() const;
}
