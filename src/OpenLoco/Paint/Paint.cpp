#include "Paint.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    static loco_global<InteractionItem, 0x00E40104> _sessionInteractionInfoType;
    static loco_global<uint8_t, 0x00E40105> _sessionInteractionInfoBh; // Unk var_29 of paintstruct
    static loco_global<int16_t, 0x00E40108> _sessionInteractionInfoX;
    static loco_global<int16_t, 0x00E4010A> _sessionInteractionInfoY;
    static loco_global<uint32_t, 0x00E4010C> _sessionInteractionInfoValue; // tileElement or thing ptr
    static loco_global<uint32_t, 0x00E40110> _getMapCoordinatesFromPosFlags;
    PaintSession _session;

    void PaintSession::init(Gfx::drawpixelinfo_t& dpi, const uint16_t viewportFlags)
    {
        _dpi = &dpi;
        _nextFreePaintStruct = &_paintEntries[0];
        _endOfPaintStructArray = &_paintEntries[3998];
        _lastPS = nullptr;
        for (auto& quadrant : _quadrants)
        {
            quadrant = nullptr;
        }
        _quadrantBackIndex = -1;
        _quadrantFrontIndex = 0;
        _lastPaintString = 0;
        _paintStringHead = 0;
    }

    // 0x0045A6CA
    PaintSession* allocateSession(Gfx::drawpixelinfo_t& dpi, uint16_t viewportFlags)
    {
        _session.init(dpi, viewportFlags);
        return &_session;
    }

    // 0x004622A2
    void PaintSession::generate()
    {
        call(0x004622A2);
    }

    // 0x0045E7B5
    void PaintSession::arrangeStructs()
    {
        call(0x0045E7B5);
    }

    // 0x0045ED91
    [[nodiscard]] InteractionArg PaintSession::getNormalInteractionInfo(const uint32_t flags)
    {
        _sessionInteractionInfoType = InteractionItem::t_0;
        _getMapCoordinatesFromPosFlags = flags;
        call(0x0045ED91);
        return InteractionArg{ _sessionInteractionInfoX, _sessionInteractionInfoY, { _sessionInteractionInfoValue }, _sessionInteractionInfoType, _sessionInteractionInfoBh };
    }

    // 0x0048DDE4
    [[nodiscard]] InteractionArg PaintSession::getStationNameInteractionInfo(const uint32_t flags)
    {
        InteractionArg interaction{};

        // -2 as there are two interaction items that you can't filter out adjust in future
        if (flags & (1 << (static_cast<uint32_t>(InteractionItem::station) - 2)))
        {
            return interaction;
        }

        auto zoom = (*_dpi)->zoom_level;
        auto left = (*_dpi)->x >> zoom;
        auto top = (*_dpi)->y >> zoom;
        auto right = ((*_dpi)->width >> zoom) + left;
        auto bottom = ((*_dpi)->height >> zoom) + top;

        for (auto& station : StationManager::stations())
        {
            if (station.empty())
            {
                continue;
            }

            if (station.flags & station_flags::flag_5)
            {
                continue;
            }

            if (top > station.label_bottom[zoom])
            {
                continue;
            }
            if (bottom < station.label_top[zoom])
            {
                continue;
            }
            if (left > station.label_right[zoom])
            {
                continue;
            }
            if (right < station.label_left[zoom])
            {
                continue;
            }

            interaction.type = InteractionItem::station;
            interaction.value = station.id();
        }

        // This is for functions that have not been implemented yet
        _sessionInteractionInfoType = interaction.type;
        _sessionInteractionInfoValue = interaction.value;
        return interaction;
    }

    // 0x0049773D
    [[nodiscard]] InteractionArg PaintSession::getTownNameInteractionInfo(const uint32_t flags)
    {
        InteractionArg interaction{};

        // -2 as there are two interaction items that you can't filter out adjust in future
        if (flags & (1 << (static_cast<uint32_t>(InteractionItem::town) - 2)))
        {
            return interaction;
        }

        auto zoom = (*_dpi)->zoom_level;
        auto left = (*_dpi)->x >> zoom;
        auto top = (*_dpi)->y >> zoom;
        auto right = ((*_dpi)->width >> zoom) + left;
        auto bottom = ((*_dpi)->height >> zoom) + top;

        for (auto& town : TownManager::towns())
        {
            if (town.empty())
            {
                continue;
            }

            if (top > town.label_bottom[zoom])
            {
                continue;
            }
            if (bottom < town.label_top[zoom])
            {
                continue;
            }
            if (left > town.label_right[zoom])
            {
                continue;
            }
            if (right < town.label_left[zoom])
            {
                continue;
            }

            interaction.type = InteractionItem::town;
            interaction.value = town.id();
        }

        // This is for functions that have not been implemented yet
        _sessionInteractionInfoType = interaction.type;
        _sessionInteractionInfoValue = interaction.value;
        return interaction;
    }
}
