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

    // 0x00461CF8
    void tilePaintSetup(PaintSession& session, const Gfx::point_t& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x00461CF8, regs);
    }

    // 0x004617C6
    void tilePaintSetup2(PaintSession& session, const Gfx::point_t& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x004617C6, regs);
    }

    // 0x0046FA88
    void entityPaintSetup(PaintSession& session, const Gfx::point_t& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x0046FA88, regs);
    }

    // 0x0046FB67
    void entityPaintSetup2(PaintSession& session, const Gfx::point_t& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x0046FB67, regs);
    }

    // 0x004622A2
    void PaintSession::generate()
    {
        if ((addr<0x00525E28, uint32_t>() & (1 << 0)) == 0)
            return;

        Gfx::drawpixelinfo_t* dpi = _dpi;
        Gfx::point_t loc = { static_cast<int16_t>(dpi->x & 0xFFE0), static_cast<int16_t>((dpi->y - 16) & 0xFFE0) };

        int16_t halfX = loc.x >> 1;
        currentRotation = Ui::WindowManager::getCurrentRotation();
        uint16_t numVerticalQuadrants = (dpi->height + currentRotation == 0 ? 1040 : 1056) >> 5;

        switch (currentRotation)
        {
            case 0:
                loc.x = loc.y - halfX;
                loc.y = loc.y + halfX;

                loc.x &= 0xFFE0;
                loc.y &= 0xFFE0;

                for (; numVerticalQuadrants > 0; --numVerticalQuadrants)
                {
                    tilePaintSetup(*this, loc);
                    entityPaintSetup(*this, loc);

                    auto loc1 = loc + Gfx::point_t{ -32, 32 };
                    tilePaintSetup2(*this, loc1);
                    entityPaintSetup(*this, loc1);

                    auto loc2 = loc + Gfx::point_t{ 0, 32 };
                    tilePaintSetup(*this, loc2);
                    entityPaintSetup(*this, loc2);

                    auto loc3 = loc + Gfx::point_t{ 32, 0 };
                    tilePaintSetup2(*this, loc3);
                    entityPaintSetup(*this, loc3);

                    auto loc4 = loc + Gfx::point_t{ 32, -32 };
                    entityPaintSetup2(*this, loc);

                    auto loc5 = loc + Gfx::point_t{ -32, 64 };
                    entityPaintSetup2(*this, loc);
                    loc += Gfx::point_t{ 32, 32 };
                }
                break;
            case 1:
                loc.x = -loc.y - halfX;
                loc.y = loc.y - halfX - 16;

                loc.x &= 0xFFE0;
                loc.y &= 0xFFE0;

                for (; numVerticalQuadrants > 0; --numVerticalQuadrants)
                {
                    tilePaintSetup(*this, loc);
                    entityPaintSetup(*this, loc);

                    auto loc1 = loc + Gfx::point_t{ -32, -32 };
                    tilePaintSetup2(*this, loc1);
                    entityPaintSetup(*this, loc1);

                    auto loc2 = loc + Gfx::point_t{ -32, 0 };
                    tilePaintSetup(*this, loc2);
                    entityPaintSetup(*this, loc2);

                    auto loc3 = loc + Gfx::point_t{ 0, 32 };
                    tilePaintSetup2(*this, loc3);
                    entityPaintSetup(*this, loc3);

                    auto loc4 = loc + Gfx::point_t{ 32, 32 };
                    entityPaintSetup2(*this, loc);

                    auto loc5 = loc + Gfx::point_t{ -64, -32 };
                    entityPaintSetup2(*this, loc);
                    loc += Gfx::point_t{ -32, 32 };
                }
                break;
            case 2:
                loc.x = -loc.y + halfX;
                loc.y = -loc.y - halfX;

                loc.x &= 0xFFE0;
                loc.y &= 0xFFE0;

                for (; numVerticalQuadrants > 0; --numVerticalQuadrants)
                {
                    tilePaintSetup(*this, loc);
                    entityPaintSetup(*this, loc);

                    auto loc1 = loc + Gfx::point_t{ 32, -32 };
                    tilePaintSetup2(*this, loc1);
                    entityPaintSetup(*this, loc1);

                    auto loc2 = loc + Gfx::point_t{ 0, -32 };
                    tilePaintSetup(*this, loc2);
                    entityPaintSetup(*this, loc2);

                    auto loc3 = loc + Gfx::point_t{ -32, 0 };
                    tilePaintSetup2(*this, loc3);
                    entityPaintSetup(*this, loc3);

                    auto loc4 = loc + Gfx::point_t{ -32, 32 };
                    entityPaintSetup2(*this, loc);

                    auto loc5 = loc + Gfx::point_t{ 32, -64 };
                    entityPaintSetup2(*this, loc);
                    loc += Gfx::point_t{ -32, -32 };
                }
                break;
            case 3:
                loc.x = loc.y + halfX;
                loc.y = -loc.y + halfX - 16;

                loc.x &= 0xFFE0;
                loc.y &= 0xFFE0;

                for (; numVerticalQuadrants > 0; --numVerticalQuadrants)
                {
                    tilePaintSetup(*this, loc);
                    entityPaintSetup(*this, loc);

                    auto loc1 = loc + Gfx::point_t{ 32, 32 };
                    tilePaintSetup2(*this, loc1);
                    entityPaintSetup(*this, loc1);

                    auto loc2 = loc + Gfx::point_t{ 32, 0 };
                    tilePaintSetup(*this, loc2);
                    entityPaintSetup(*this, loc2);

                    auto loc3 = loc + Gfx::point_t{ 0, -32 };
                    tilePaintSetup2(*this, loc3);
                    entityPaintSetup(*this, loc3);

                    auto loc4 = loc + Gfx::point_t{ -32, -32 };
                    entityPaintSetup2(*this, loc);

                    auto loc5 = loc + Gfx::point_t{ 64, 32 };
                    entityPaintSetup2(*this, loc);
                    loc += Gfx::point_t{ 32, -32 };
                }
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
