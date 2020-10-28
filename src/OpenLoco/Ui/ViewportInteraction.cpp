#include "../CompanyManager.h"
#include "../Config.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
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

    static bool _station(InteractionArg& interaction);
    static bool _station(station_id_t id);

    // 0x004CD95A
    static bool _track(InteractionArg& interaction)
    {
        Map::track_element* track = reinterpret_cast<Map::track_element*>(interaction.object);
        if (!track->hasStationElement())
            return false;

        auto station = reinterpret_cast<Map::station_element*>(track++);
        if (station->isFlag5())
            return false;

        interaction.type = InteractionItem::station;
        interaction.object = station;
        return _station(interaction);
    }

    // 0x004CD974
    static bool _road(InteractionArg& interaction)
    {
        Map::road_element* road = reinterpret_cast<Map::road_element*>(interaction.object);
        if (!road->hasStationElement())
            return false;

        Map::station_element* station = nullptr;
        Map::tile tile{ interaction.x, interaction.y, reinterpret_cast<Map::tile_element*>(road) };
        for (auto t : tile)
        {
            station = t.asStation();
            if (station != nullptr)
            {
                break;
            }
        }

        if (station == nullptr)
        {
            return false;
        }

        interaction.object = station;
        interaction.type = InteractionItem::dock;
        if (station->isFlag5())
            return false;

        interaction.type = InteractionItem::station;
        return _station(interaction);
    }

    // 0x004CD99A
    static bool _station(InteractionArg& interaction)
    {
        Map::station_element* station = reinterpret_cast<Map::station_element*>(interaction.value);
        if (station->isFlag4())
            return false;

        interaction.value = station->stationId();
        return _station(station->stationId());
    }

    static loco_global<uint16_t, 0x00F252A4> _hoveredStationId;

    // 0x004CD9B0
    static bool _station(const station_id_t id)
    {
        _hoveredStationId = id;

        auto station = StationManager::get(id);

        Input::setMapSelectionFlags(Input::MapSelectionFlags::unk_6);
        ViewportManager::invalidate(station);
        Windows::MapToolTip::setOwner(station->owner);
        auto args = Windows::MapToolTip::getArguments();
        args.push(StringIds::stringid_stringid_wcolour3_stringid);
        args.push(station->name);
        args.push(station->town);
        args.push(getTransportIconsFromStationFlags(station->flags));
        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        buffer = station->getStatusString(buffer);

        buffer = StringManager::formatString(buffer, StringIds::station_accepts);
        bool seperator = false; // First cargo item does not need a seperator
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            auto& stats = station->cargo_stats[cargoId];

            if (!stats.isAccepted())
            {
                continue;
            }

            if (seperator)
            {
                buffer = StringManager::formatString(buffer, StringIds::unit_separator);
            }
            buffer = StringManager::formatString(buffer, ObjectManager::get<cargo_object>(cargoId)->name);
            seperator = true;
        }

        args.push(StringIds::buffer_338);

        return true;
    }

    // 0x004CD7FB
    static bool _town(const town_id_t id)
    {
        auto town = TownManager::get(id);

        auto args = Windows::MapToolTip::getArguments();
        args.push(StringIds::wcolour3_stringid_2);
        args.push(town->name); // args + 4 empty
        args.skip(2);
        args.push(StringIds::town_size_and_population);
        args.push(town->getTownSizeString());
        args.push(town->population);

        return true;
    }

    // 0x004CD8D5
    static bool _industry(InteractionArg& interaction)
    {
        auto industryTile = reinterpret_cast<Map::industry_element*>(interaction.value);
        if (industryTile->isFlag4())
            return false;

        interaction.value = industryTile->industryId();
        auto industry = industryTile->industry();

        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        *buffer = 0;
        industry->getStatusString(buffer);
        auto args = Windows::MapToolTip::getArguments();
        if (std::strlen(buffer) != 0)
        {
            args.push(StringIds::wcolour3_stringid_2);
            args.push(industry->name);
            args.push(industry->town);
            args.push(StringIds::buffer_338);
        }
        else
        {
            args.push(industry->name);
            args.push(industry->town);
        }
        return true;
    }

    // 0x004CDA7C
    static bool _vehicle(const InteractionArg& interaction)
    {
        Thing* thing = reinterpret_cast<Thing*>(interaction.object);
        auto vehicle = reinterpret_cast<OpenLoco::vehicle*>(thing->asVehicle());
        if (vehicle == nullptr)
        {
            return false;
        }
        auto head = ThingManager::get<vehicle_head>(vehicle->head);

        auto company = CompanyManager::get(head->owner);
        Windows::MapToolTip::setOwner(head->owner);
        auto status = head->getStatus();
        auto args = Windows::MapToolTip::getArguments();
        if (status.status2 == StringIds::null)
        {
            args.push(StringIds::wcolour3_stringid);
        }
        else
        {
            args.push(StringIds::wcolour3_stringid_stringid);
        }
        args.push(CompanyManager::getControllingId() == head->owner ? StringIds::company_vehicle : StringIds::competitor_vehicle);
        args.push(company->name); // args + 6 is empty
        args.skip(2);
        args.push(head->var_22);
        args.push(head->var_44);
        args.push(status.status1);
        args.push(status.status1Args); //32bit
        args.push(status.status2);
        args.push(status.status2Args); //32bit
        return true;
    }

    // 0x004CD84A
    bool _headquarter(const InteractionArg& interaction)
    {
        auto buildingTile = reinterpret_cast<Map::building_element*>(interaction.value);
        if (buildingTile->isFlag4())
        {
            return false;
        }

        const auto index = buildingTile->multiTileIndex();
        const Map::map_pos3 pos = { interaction.x - Map::offsets[index].x,
                                    interaction.y - Map::offsets[index].y,
                                    buildingTile->baseZ() };

        for (auto& company : CompanyManager::companies())
        {
            if (company.empty())
            {
                continue;
            }

            if (company.headquarters_x != pos.x || company.headquarters_y != pos.y || company.headquarters_z != pos.z)
            {
                continue;
            }

            Windows::MapToolTip::setOwner(company.id());
            auto args = Windows::MapToolTip::getArguments();
            args.push(StringIds::wcolour3_stringid_2);
            args.push(company.name);
            args.skip(2);
            args.push(StringIds::headquarters);
            return true;
        }
        return false;
    }

    std::optional<uint32_t> vehicleDistanceFromLocation(const vehicle_base& component, const viewport_pos& targetPosition)
    {
        ViewportRect rect = {
            component.sprite_left,
            component.sprite_top,
            component.sprite_bottom,
            component.sprite_right
        };
        if (rect.contains(targetPosition))
        {
            uint32_t xDiff = std::abs(targetPosition.x - (component.sprite_right + component.sprite_left) / 2);
            uint32_t yDiff = std::abs(targetPosition.y - (component.sprite_top + component.sprite_bottom) / 2);
            return xDiff + yDiff;
        }
        return {};
    }

    void checkAndSetNearestVehicle(uint32_t& nearestDistance, vehicle_base*& nearestVehicle, vehicle_base& checkVehicle, const viewport_pos& targetPosition)
    {
        if (checkVehicle.sprite_left != 0x8000)
        {
            auto distanceRes = vehicleDistanceFromLocation(checkVehicle, targetPosition);
            if (distanceRes)
            {
                if (*distanceRes < nearestDistance)
                {
                    nearestDistance = *distanceRes;
                    nearestVehicle = &checkVehicle;
                }
            }
        }
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

        bool success = false;
        switch (interaction.type)
        {
            case InteractionItem::track: // 4
                success = _track(interaction);
                break;

            case InteractionItem::road: // 16
                success = _road(interaction);
                break;
            case InteractionItem::town: // 14
                success = _town(static_cast<town_id_t>(interaction.value));
                break;
            case InteractionItem::station: // 15
                success = _station(static_cast<station_id_t>(interaction.value));
                break;
            case InteractionItem::trackStation: // 7
            case InteractionItem::roadStation:  // 8
            case InteractionItem::airport:      // 9
            case InteractionItem::dock:         // 10
                success = _station(interaction);
                break;
            case InteractionItem::industry: // 20
                success = _industry(interaction);
                break;
            case InteractionItem::headquarterBuilding: // 21
                success = _headquarter(interaction);
                break;
            case InteractionItem::thing: // 3
                success = _vehicle(interaction);
                break;
        }
        if (success == true)
        {
            return interaction;
        }
        auto window = WindowManager::findAt(tempX, tempY);
        if (window == nullptr)
            return InteractionArg{};

        auto viewport = window->viewports[0];
        if (viewport == nullptr)
            return InteractionArg{};

        if (viewport->zoom > Config::get().vehicles_min_scale)
            return InteractionArg{};

        uint32_t nearestDistance = std::numeric_limits<uint32_t>().max();
        vehicle_base* nearestVehicle = nullptr;
        auto targetPosition = viewport->uiToMap({ tempX, tempY });

        for (auto v : ThingManager::VehicleList())
        {
            auto train = Things::Vehicle::Vehicle(v);
            checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *train.veh2, targetPosition);
            for (auto car : train.cars)
            {
                for (auto carComponent : car)
                {
                    checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *carComponent.front, targetPosition);
                    checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *carComponent.back, targetPosition);
                    checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *carComponent.body, targetPosition);
                }
            }
        }

        if (nearestDistance <= 32)
        {
            interaction.type = InteractionItem::thing;
            interaction.object = reinterpret_cast<void*>(nearestVehicle);
            interaction.x = nearestVehicle->x;
            interaction.y = nearestVehicle->y;

            // 4CDA7C aka _vehicle
            _vehicle(interaction);
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
