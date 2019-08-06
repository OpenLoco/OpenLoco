#include "../Config.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringManager.h"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../Paint/Paint.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../TownManager.h"
#include "../Ui.h"
#include "../Ui/ScrollView.h"
#include "../ViewportManager.h"
#include "../Window.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ViewportInteraction
{
    InteractionArg::InteractionArg(const Paint::PaintStruct& ps)
        : x(ps.map_x)
        , y(ps.map_y)
        , object(ps.thing)
        , type(ps.type)
        , unkBh(ps.var_29)
    {
    }

    static bool _station(Map::station_element* station);
    static bool _station(station_id_t id);

    static bool _track(Map::track_element* track)
    {
        if (!track->hasStationElement())
            return false;

        auto next = (Map::station_element*)track++;
        if (next->isFlag5())
            return false;

        return _station(next);
    }

    // 0x004CD974
    static bool _road(Map::road_element* road)
    {
        if (!road->hasStationElement())
            return false;

        Map::station_element* station;
        for (auto tile = (Map::tile_element*)road++; !tile->isLast(); tile++)
        {
            if (tile->type() == Map::element_type::station)
            {
                station = tile->asStation();
                break;
            }
        }

        if (station->isFlag5())
            return false;

        return _station(station);
    }

    // 0x004CD99A
    static bool _station(Map::station_element* station)
    {
        if (station->isFlag4())
            return false;

        return _station(station->stationId());
    }

    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    static loco_global<uint16_t, 0x00F252A4> _hoveredStationId;
    static loco_global<company_id_t, 0x0050A040> _mapTooltipOwner;

    // 0x004CD9B0
    static bool _station(station_id_t id)
    {
        _hoveredStationId = id;

        auto station = StationManager::get(id);

        *_mapSelectionFlags |= (1 << 6);
        ViewportManager::invalidate(station);

        _mapTooltipOwner = station->owner;

        args + 2 = station->name;
        args + 4 = station->town;
        args + 6 = label_icons[station->flags & 0x0F];
        args + 10 = town->population; // 32-bit

        station->getStatusString(); // save to buffer 338

        string_id prefix = 1444; // "{NEWLINE}Accepts "
        for (int i = 0; i < max_cargo_stats; i++)
        {
            auto& cargo = station->cargo_stats[i];
            if (cargo.isAccepted())
            {
                out += StringManager::formatString(prefix);
                out += StringManager::formatString(ObjectManager::get<cargo_object>(i)->name);

                prefix = StringIds::unit_separator;
            }
        }

        args + 8 = StringIds::buffer_338;
        args + 0 = 1239; //'{STRINGID} {STRINGID}{NEWLINE}{COLOUR WINDOW_3}{STRINGID}'

        return true;
    }

    static bool _town(town_id_t id)
    {
        auto town = TownManager::get(id);

        args + 2 = town->name;
        args + 6 = 1304;              // "{STRINGID} population {INT32}"
        args + 8 = 617 + town->size;  // 'hamlet' etc;
        args + 10 = town->population; // 32-bit
        args + 0 = 627;               // "{STRINGID}{NEWLINE}{COLOUR WINDOW_3}{STRINGID}"

        return true;
    }

    // 0x0045935F

    /**
     *
     * @param id @<dx>
     */
    static std::string sub_0045935F(industry_id_t id)
    {
        auto industry = IndustryManager::get(id);

        if (industry->flags & IndustryFlags::closing_down)
        {
            return 1419; // Closing down
        }

        if (industry->under_construction != 0xFF)
        {
            return 1366; // Under construction
        }

        auto object = industry->object();

        string_id produces = 1367; // "Producing "
        std::string out;

        if (object->required_cargo_type[0] != -1 || object->required_cargo_type[1] != -1 || object->required_cargo_type[2] != -1)
        {
            out += StringManager::formatString(1371); // "Requires "

            string_id voegwoord = 1374; // " or "
            if (object->flags & 0x20000)
            {
                voegwoord = 1373; // " and "
            }

            bool dl = false;
            for (int i = 0; i < 3; i++)
            {
                if (object->required_cargo_type[i] != -1)
                {
                    if (dl)
                    {
                        out += StringManager::formatString(1373); // " and "
                    }
                    out += StringManager::formatString(ObjectManager::get<cargo_object>(object->required_cargo_type[i])->name); // "Producing "
                    dl = true;
                }
            }

            produces = 1370; // " to produce "
        }

        if (object->produced_cargo_type[0] != -1 || object->produced_cargo_type[1] != -1)
        {
            out += StringManager::formatString(produces);

            bool dl = false;
            for (int i = 0; i < 2; i++)
            {
                if (object->produced_cargo_type[i] != -1)
                {
                    if (dl)
                    {
                        out += StringManager::formatString(1373); // " and "
                    }
                    out += StringManager::formatString(ObjectManager::get<cargo_object>(object->produced_cargo_type[i])->name); // "Producing "
                    dl = true;
                }
            }
        }
    }

    static bool _industry(Map::tree_element* tile)
    {
        if (tile->isFlag4())
            return false;

        auto industry = tile->industry();

        args + 2 = industry->name;
        args + 4 = industry->town; // likely town id
        args + 6 = StringIds::buffer_338;
        args + 0 = 627; // "{STRINGID}{NEWLINE}{COLOUR WINDOW_3}{STRINGID}"
    }

    void _vehicle(OpenLoco::vehicle* vehicle)
    {
        ThingManager::get<vehicle>(vehicle->head);

        if (vehicle->tile_x == -1)
        {
            bx = 473;
            cx = StringIds::null;
        }
        else if (false)
        {
            bx = 464;
            cx = StringIds::null;
        }
        else
        {
        }
    }

    void _headquarter(Map::tile_element* tile)
    {
    }

    // 0x004CD658
    InteractionArg getItemLeft(int16_t tempX, int16_t tempY)
    {
        if (OpenLoco::isTitleMode())
            return InteractionArg{};

        // 0x2000 | 0x1000 | 0x2
        // station | town | thing
        auto res = getMapCoordinatesFromPos(tempX, tempY, 0b11111111111111111100111111111101);
        auto interaction = res.first;
        if (interaction.type != InteractionItem::thing)
        {
            // 0x10000 | 0x2000 | 0x1000 | 0x800 | 0x200 | 0x20 | 0x4 | 0x2
            // industry | station | town | (7|8|9|10) | headquarterBuilding | road | track | thing
            res = getMapCoordinatesFromPos(tempX, tempY, 0b11111111111111101100010111011001);
            interaction = res.first;
        }

        switch (interaction.type)
        {
            case InteractionItem::track: // 4
                return _track(interaction.value);

            case InteractionItem::road: // 16
                return _road(interaction.value);

            case InteractionItem::town: // 14
                return _town((town_id_t)interaction.value);

            case InteractionItem::station: // 15
                return _station((station_id_t)interaction.value);

            case InteractionItem::trackStation: // 7
            case InteractionItem::roadStation:  // 8
            case InteractionItem::airport:      // 9
            case InteractionItem::dock:         // 10
                return _station((Map::station_element*)interaction.value);

            case InteractionItem::industry: // 20
                return _industry(interaction.value);

            case InteractionItem::headquarterBuilding: // 21
                return _headquarter(interaction.value);

            case InteractionItem::thing: // 3
            {
                auto t = (Thing*)interaction.value;
                auto vehicle = t->asVehicle();
                if (vehicle == nullptr)
                    break;

                return _vehicle(vehicle);
            }
        }

        auto window = WindowManager::findAt(tempX, tempY);
        if (window == nullptr)
            return InteractionArg{};

        auto viewport = window->viewports[0];
        if (viewport == nullptr)
            return InteractionArg{};

        if (viewport->zoom > Config::get().vehicles_min_scale)
            return InteractionArg{};

        uint16_t bp = std::numeric_limits<uint16_t>().max();

        vehicle_base* esi;

        for (auto v : ThingManager::VehicleList())
        {
            auto train = Things::Vehicle::Vehicle(v);
            for (auto car : train.cars)
            {
                for (auto carComponent : car)
                {
                    // Loop through all components including veh2
                    if (carComponent.front->sprite_left == 0x8000)
                        continue;

                    // determine distance from click; save to bp
                }
            }
        }

        if (bp <= 32)
        {
            interaction.type = InteractionItem::thing;
            interaction.object = esi;
            interaction.x = esi->x;
            interaction.y = esi->y;

            // 4CDA7C aka _vehicle
            return interaction;
        }

        return InteractionArg{};
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
                auto townInteraction = session->getTownNameInteractionInfo(flags);
                if (townInteraction.type != InteractionItem::t_0)
                {
                    interaction = townInteraction;
                }
            }
            break;
        }
        _50BF68 = 0;
        return std::make_pair(interaction, chosenV);
    }
}
