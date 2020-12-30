#include "Paint.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"

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

    void registerHooks()
    {
        registerHook(
            0x004622A2,
            [](registers& regs) -> uint8_t {
                registers backup = regs;

                PaintSession session;
                session.generate();

                regs = backup;
                return 0;
            });
    }

    // 0x00461CF8
    static void paintTileElements(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x00461CF8, regs);
    }

    // 0x004617C6
    static void paintTileElements2(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x004617C6, regs);
    }

    // 0x0046FA88
    static void paintEntities(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x0046FA88, regs);
    }

    // 0x0046FB67
    static void paintEntities2(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x0046FB67, regs);
    }

    template<uint8_t rotation>
    void generateRotated(PaintSession& session)
    {
        auto* dpi = session.getContext();
        uint16_t numVerticalQuadrants = (dpi->height + (rotation == 0 ? 1040 : 1056)) >> 5;
        auto mapLoc = Ui::viewportCoordToMapCoord(static_cast<int16_t>(dpi->x & 0xFFE0), static_cast<int16_t>((dpi->y - 16) & 0xFFE0), 0, rotation);
        if constexpr (rotation & 1)
        {
            mapLoc.y -= 16;
        }
        mapLoc.x &= 0xFFE0;
        mapLoc.y &= 0xFFE0;
        constexpr uint8_t rotOrder[] = { 0, 3, 2, 1 };
        constexpr Map::map_pos additionalQuadrants[5] = {
            Map::map_pos{ -32, 32 }.rotate(rotOrder[rotation]),
            Map::map_pos{ 0, 32 }.rotate(rotOrder[rotation]),
            Map::map_pos{ 32, 0 }.rotate(rotOrder[rotation]),
            Map::map_pos{ 32, -32 }.rotate(rotOrder[rotation]),
            Map::map_pos{ -32, 64 }.rotate(rotOrder[rotation]),
        };
        constexpr auto nextVerticalQuadrant = Map::map_pos{ 32, 32 }.rotate(rotOrder[rotation]);

        for (; numVerticalQuadrants > 0; --numVerticalQuadrants)
        {
            paintTileElements(session, mapLoc);
            paintEntities(session, mapLoc);

            auto loc1 = mapLoc + additionalQuadrants[0];
            paintTileElements2(session, loc1);
            paintEntities(session, loc1);

            auto loc2 = mapLoc + additionalQuadrants[1];
            paintTileElements(session, loc2);
            paintEntities(session, loc2);

            auto loc3 = mapLoc + additionalQuadrants[2];
            paintTileElements2(session, loc3);
            paintEntities(session, loc3);

            auto loc4 = mapLoc + additionalQuadrants[3];
            paintEntities2(session, loc4);

            auto loc5 = mapLoc + additionalQuadrants[4];
            paintEntities2(session, loc5);

            mapLoc += nextVerticalQuadrant;
        }
    }

    // 0x004622A2
    void PaintSession::generate()
    {
        if ((addr<0x00525E28, uint32_t>() & (1 << 0)) == 0)
            return;

        currentRotation = Ui::WindowManager::getCurrentRotation();
        switch (currentRotation)
        {
            case 0:
                generateRotated<0>(*this);
                break;
            case 1:
                generateRotated<1>(*this);
                break;
            case 2:
                generateRotated<2>(*this);
                break;
            case 3:
                generateRotated<3>(*this);
                break;
        }
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

        auto rect = (*_dpi)->getDrawableRect();

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

            if (!station.labelPosition.contains(rect, (*_dpi)->zoom_level))
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

        auto rect = (*_dpi)->getDrawableRect();

        for (auto& town : TownManager::towns())
        {
            if (town.empty())
            {
                continue;
            }

            if (!town.labelPosition.contains(rect, (*_dpi)->zoom_level))
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
