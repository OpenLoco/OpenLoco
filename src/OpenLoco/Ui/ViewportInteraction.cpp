#include "../Config.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringManager.h"
#include "../Paint/Paint.h"
#include "../StationManager.h"
#include "../Ui.h"
#include "../Ui/ScrollView.h"
#include "../Window.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ViewportInteraction
{
    // 0x004CD658
    InteractionArg getItemLeft(int16_t tempX, int16_t tempY)
    {
        registers regs;
        regs.ax = tempX;
        regs.bx = tempY;

        call(0x004CD658, regs);
        InteractionArg result;
        result.value = regs.edx;
        result.x = regs.ax;
        result.y = regs.cx;
        result.unkBh = regs.bh;
        result.type = static_cast<InteractionItem>(regs.bl);

        return result;
    }

    // 0x004CDB2B
    InteractionArg rightOver(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CDB2B, regs);
        InteractionArg result;
        result.value = regs.edx;
        result.x = regs.ax;
        result.y = regs.cx;
        result.unkBh = regs.bh;
        result.type = static_cast<InteractionItem>(regs.bl);

        return result;
    }

    // 0x00459E54
    std::pair<ViewportInteraction::InteractionArg, viewport*> getMapCoordinatesFromPos(int32_t screenX, int32_t screenY, int32_t flags)
    {
        static loco_global<uint8_t, 0x0050BF68> _50BF68; // If in get map coords
        static loco_global<Gfx::drawpixelinfo_t, 0x00E0C3E4> _dpi1;
        static loco_global<Gfx::drawpixelinfo_t, 0x00E0C3F4> _dpi2;

        _50BF68 = 1;
        ViewportInteraction::InteractionArg interaction{};
        Gfx::point_t screenPos = { static_cast<int16_t>(screenX), static_cast<int16_t>(screenY) };
        auto w = WindowManager::findAt(screenPos);
        if (w == nullptr)
        {
            _50BF68 = 0;
            return std::make_pair(interaction, nullptr);
        }

        viewport* chosenV = nullptr;
        for (auto vp : w->viewports)
        {
            if (vp == nullptr)
                continue;

            if (!vp->containsUi({ screenPos.x, screenPos.y }))
                continue;

            chosenV = vp;
            auto vpPos = vp->uiToMap({ screenPos.x, screenPos.y });
            _dpi1->zoom_level = vp->zoom;
            _dpi1->x = (0xFFFF << vp->zoom) & vpPos.x;
            _dpi1->y = (0xFFFF << vp->zoom) & vpPos.y;
            _dpi2->x = _dpi1->x;
            _dpi2->y = _dpi1->y;
            _dpi2->width = 1;
            _dpi2->height = 1;
            _dpi2->zoom_level = _dpi1->zoom_level;
            auto* session = Paint::allocateSession(_dpi2, vp->flags);
            session->generate();
            session->arrangeStructs();
            interaction = session->getNormalInteractionInfo(flags);
            if (!(vp->flags & ViewportFlags::station_names_displayed))
            {
                if (_dpi2->zoom_level <= Config::get().station_names_min_scale)
                {
                    auto stationInteraction = session->getStationNameInteractionInfo(flags);
                    if (stationInteraction.type != InteractionItem::t_0)
                    {
                        interaction = stationInteraction;
                    }
                }
            }
            if (!(vp->flags & ViewportFlags::town_names_displayed))
            {
                interaction = session->getTownNameInteractionInfo(flags);
            }
            break;
        }
        _50BF68 = 0;
        return std::make_pair(interaction, chosenV);
    }
}
