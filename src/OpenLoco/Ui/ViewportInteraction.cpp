#include "../CompanyManager.h"
#include "../Config.h"
#include "../Entities/EntityManager.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Localisation/StringManager.h"
#include "../Map/TileManager.h"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../Paint/Paint.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui.h"
#include "../Ui/ScrollView.h"
#include "../Vehicles/Vehicle.h"
#include "../ViewportManager.h"
#include "../Window.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ViewportInteraction
{
    InteractionArg::InteractionArg(const Paint::PaintStruct& ps)
        : object(ps.entity)
        , type(ps.type)
        , unkBh(ps.var_29)
    {
        pos = Pos2{ ps.map_x, ps.map_y };
    }

    static bool getStationArguments(InteractionArg& interaction);
    static bool getStationArguments(StationId_t id);

    // 0x004CD95A
    static bool _track(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* track = tileElement->asTrack();
        if (track == nullptr)
            return false;
        if (!track->hasStationElement())
            return false;

        tileElement++;
        auto* station = tileElement->asStation();
        if (station == nullptr)
            return false;
        if (station->isFlag5())
            return false;

        interaction.type = InteractionItem::trackStation;
        interaction.object = station;
        return getStationArguments(interaction);
    }

    // 0x004CD974
    static bool _road(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* road = tileElement->asRoad();
        if (road == nullptr)
            return false;
        if (!road->hasStationElement())
            return false;

        Map::StationElement* station = nullptr;
        Map::Tile tile{ interaction.pos, tileElement };
        for (auto& t : tile)
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

        interaction.type = InteractionItem::roadStation;
        return getStationArguments(interaction);
    }

    // 0x004CD99A
    static bool getStationArguments(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* station = tileElement->asStation();
        if (station == nullptr)
            return false;
        if (station->isGhost())
            return false;

        interaction.value = station->stationId();
        interaction.type = InteractionItem::stationLabel;
        return getStationArguments(station->stationId());
    }

    static loco_global<uint16_t, 0x00F252A4> _hoveredStationId;

    // 0x004CD9B0
    static bool getStationArguments(const StationId_t id)
    {
        _hoveredStationId = id;

        auto station = StationManager::get(id);

        Input::setMapSelectionFlags(Input::MapSelectionFlags::unk_6);
        ViewportManager::invalidate(station);
        Windows::MapToolTip::setOwner(station->owner);
        auto args = FormatArguments::mapToolTip(StringIds::stringid_stringid_wcolour3_stringid);
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
            buffer = StringManager::formatString(buffer, ObjectManager::get<CargoObject>(cargoId)->name);
            seperator = true;
        }

        args.push(StringIds::buffer_338);

        return true;
    }

    // 0x004CD7FB
    static bool getTownArguments(const TownId_t id)
    {
        auto town = TownManager::get(id);

        auto args = FormatArguments::mapToolTip(StringIds::wcolour3_stringid_2, town->name); // args + 4 empty
        args.skip(2);
        args.push(StringIds::town_size_and_population);
        args.push(town->getTownSizeString());
        args.push(town->population);

        return true;
    }

    // 0x004CD8D5
    static bool getIndustryArguments(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* industryTile = tileElement->asIndustry();
        if (industryTile == nullptr)
            return false;
        if (industryTile->isGhost())
            return false;

        interaction.value = industryTile->industryId();
        auto industry = industryTile->industry();

        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        *buffer = 0;
        industry->getStatusString(buffer);
        auto args = FormatArguments::mapToolTip();
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
    static bool getVehicleArguments(const InteractionArg& interaction)
    {
        auto* entity = reinterpret_cast<EntityBase*>(interaction.object);
        auto vehicle = entity->asVehicle();
        if (vehicle == nullptr)
        {
            return false;
        }
        auto head = EntityManager::get<Vehicles::VehicleHead>(vehicle->getHead());

        auto company = CompanyManager::get(head->owner);
        Windows::MapToolTip::setOwner(head->owner);
        auto status = head->getStatus();
        auto args = FormatArguments::mapToolTip();
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
        args.push(head->name);
        args.push(head->ordinalNumber);
        args.push(status.status1);
        args.push(status.status1Args); //32bit
        args.push(status.status2);
        args.push(status.status2Args); //32bit
        return true;
    }

    // 0x004CD84A
    static bool getHeadquarterArguments(const InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* buildingTile = tileElement->asBuilding();
        if (buildingTile == nullptr)
        {
            return false;
        }
        if (buildingTile->isGhost())
        {
            return false;
        }

        const auto index = buildingTile->multiTileIndex();
        const auto firstTile = interaction.pos - Map::offsets[index];
        const Map::Pos3 pos = { firstTile.x, firstTile.y, buildingTile->baseZ() };

        for (auto& company : CompanyManager::companies())
        {
            if (company.headquarters_x != pos.x || company.headquarters_y != pos.y || company.headquarters_z != pos.z)
            {
                continue;
            }

            Windows::MapToolTip::setOwner(company.id());
            auto args = FormatArguments::mapToolTip(StringIds::wcolour3_stringid_2, company.name);
            args.skip(2);
            args.push(StringIds::headquarters);
            return true;
        }
        return false;
    }

    static std::optional<uint32_t> vehicleDistanceFromLocation(const Vehicles::VehicleBase& component, const viewport_pos& targetPosition)
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

    static void checkAndSetNearestVehicle(uint32_t& nearestDistance, Vehicles::VehicleBase*& nearestVehicle, Vehicles::VehicleBase& checkVehicle, const viewport_pos& targetPosition)
    {
        if (checkVehicle.sprite_left != Location::null)
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

        auto interactionsToInclude = ~(InteractionItemFlags::entity | InteractionItemFlags::townLabel | InteractionItemFlags::stationLabel);
        auto res = getMapCoordinatesFromPos(tempX, tempY, interactionsToInclude);

        auto interaction = res.first;
        if (interaction.type != InteractionItem::entity)
        {
            // clang-format off
            interactionsToInclude = ~(InteractionItemFlags::entity | InteractionItemFlags::track | InteractionItemFlags::roadAndTram
                | InteractionItemFlags::headquarterBuilding | InteractionItemFlags::station | InteractionItemFlags::townLabel
                | InteractionItemFlags::stationLabel | InteractionItemFlags::industry);
            // clang-format on
            res = getMapCoordinatesFromPos(tempX, tempY, interactionsToInclude);
            interaction = res.first;
        }

        // TODO: Rework so that getting the interaction arguments and getting the map tooltip format arguments are seperated
        bool success = false;
        switch (interaction.type)
        {
            case InteractionItem::track:
                success = _track(interaction);
                break;

            case InteractionItem::road:
                success = _road(interaction);
                break;
            case InteractionItem::townLabel:
                success = getTownArguments(static_cast<TownId_t>(interaction.value));
                break;
            case InteractionItem::stationLabel:
                success = getStationArguments(static_cast<StationId_t>(interaction.value));
                break;
            case InteractionItem::trackStation:
            case InteractionItem::roadStation:
            case InteractionItem::airport:
            case InteractionItem::dock:
                success = getStationArguments(interaction);
                break;
            case InteractionItem::industry:
                success = getIndustryArguments(interaction);
                break;
            case InteractionItem::headquarterBuilding:
                success = getHeadquarterArguments(interaction);
                break;
            case InteractionItem::entity:
                success = getVehicleArguments(interaction);
                break;
            default:
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
        Vehicles::VehicleBase* nearestVehicle = nullptr;
        auto targetPosition = viewport->uiToMap({ tempX, tempY });

        for (auto v : EntityManager::VehicleList())
        {
            auto train = Vehicles::Vehicle(v);
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

        if (nearestDistance <= 32 && nearestVehicle != nullptr)
        {
            interaction.type = InteractionItem::entity;
            interaction.object = reinterpret_cast<void*>(nearestVehicle);
            interaction.pos = nearestVehicle->position;

            getVehicleArguments(interaction);
            return interaction;
        }

        return InteractionArg{};
    }

    // 0x004CDB2B
    InteractionArg rightOver(int16_t x, int16_t y)
    {
        if (OpenLoco::isTitleMode())
            return InteractionArg{};

        // Interaction types to exclude by default
        auto interactionsToExclude = 0 | InteractionItemFlags::surface | InteractionItemFlags::water;

        // TODO: Handle in the paint functions
        // Get the viewport and add extra flags for hidden scenery
        auto screenPos = Gfx::point_t(x, y);
        auto w = WindowManager::findAt(screenPos);
        if (w != nullptr)
        {
            for (auto vp : w->viewports)
            {
                if (vp != nullptr && vp->containsUi({ screenPos.x, screenPos.y }))
                {
                    if (vp->flags & ViewportFlags::hide_foreground_scenery_buildings)
                    {
                        interactionsToExclude |= InteractionItemFlags::building | InteractionItemFlags::headquarterBuilding | InteractionItemFlags::industry | InteractionItemFlags::tree | InteractionItemFlags::wall;
                    }
                }
            }
        }

        registers regs;
        regs.ax = x;
        regs.bx = y;
        regs.edx = interactionsToExclude;

        call(0x004CDB3F, regs);
        InteractionArg result;
        result.value = regs.edx;
        result.pos.x = regs.ax;
        result.pos.y = regs.cx;
        result.unkBh = regs.bh;
        result.type = static_cast<InteractionItem>(regs.bl);

        return result;
    }

    // 0x00459E54
    std::pair<ViewportInteraction::InteractionArg, Viewport*> getMapCoordinatesFromPos(int32_t screenX, int32_t screenY, int32_t flags)
    {
        static loco_global<uint8_t, 0x0050BF68> _50BF68; // If in get map coords
        static loco_global<Gfx::Context, 0x00E0C3E4> _context1;
        static loco_global<Gfx::Context, 0x00E0C3F4> _context2;

        _50BF68 = 1;
        ViewportInteraction::InteractionArg interaction{};
        Gfx::point_t screenPos = { static_cast<int16_t>(screenX), static_cast<int16_t>(screenY) };
        auto w = WindowManager::findAt(screenPos);
        if (w == nullptr)
        {
            _50BF68 = 0;
            return std::make_pair(interaction, nullptr);
        }

        Viewport* chosenV = nullptr;
        for (auto vp : w->viewports)
        {
            if (vp == nullptr)
                continue;

            if (!vp->containsUi({ screenPos.x, screenPos.y }))
                continue;

            chosenV = vp;
            auto vpPos = vp->uiToMap({ screenPos.x, screenPos.y });
            _context1->zoom_level = vp->zoom;
            _context1->x = (0xFFFF << vp->zoom) & vpPos.x;
            _context1->y = (0xFFFF << vp->zoom) & vpPos.y;
            _context2->x = _context1->x;
            _context2->y = _context1->y;
            _context2->width = 1;
            _context2->height = 1;
            _context2->zoom_level = _context1->zoom_level;
            auto* session = Paint::allocateSession(_context2, vp->flags);
            session->generate();
            session->arrangeStructs();
            interaction = session->getNormalInteractionInfo(flags);
            if (!(vp->flags & ViewportFlags::station_names_displayed))
            {
                if (_context2->zoom_level <= Config::get().station_names_min_scale)
                {
                    auto stationInteraction = session->getStationNameInteractionInfo(flags);
                    if (stationInteraction.type != InteractionItem::noInteraction)
                    {
                        interaction = stationInteraction;
                    }
                }
            }
            if (!(vp->flags & ViewportFlags::town_names_displayed))
            {
                auto townInteraction = session->getTownNameInteractionInfo(flags);
                if (townInteraction.type != InteractionItem::noInteraction)
                {
                    interaction = townInteraction;
                }
            }
            break;
        }
        _50BF68 = 0;
        return std::make_pair(interaction, chosenV);
    }

    // 0x00460781
    // regs.ax = screenCoords.x;
    // regs.bx = screenCoords.y;
    // returns
    // regs.edx = InteractionInfo.value (unsure if ever used)
    // regs.ax = mapX, 0x8000 - in case of failure
    // regs.bx = mapY
    // regs.ecx = closestEdge (unsure if ever used)
    std::optional<Pos2> getTileStartAtCursor(const xy32& screenCoords)
    {
        auto [info, viewport] = getMapCoordinatesFromPos(screenCoords.x, screenCoords.y, ~(InteractionItemFlags::surface | InteractionItemFlags::water));

        if (info.type == InteractionItem::noInteraction)
        {
            return {};
        }

        int16_t waterHeight = 0; // E40130
        if (info.type == InteractionItem::water)
        {
            auto* surface = static_cast<const SurfaceElement*>(info.object);
            waterHeight = surface->water() * 16;
        }

        const auto minPosition = info.pos;                  // E40128/A
        const auto maxPosition = info.pos + Pos2{ 31, 31 }; // E4012C/E
        auto mapPos = info.pos + Pos2{ 16, 16 };
        const auto initialVPPos = viewport->uiToMap(screenCoords);

        for (int32_t i = 0; i < 5; i++)
        {
            int16_t z = waterHeight;
            if (info.type != InteractionItem::water)
            {
                z = TileManager::getHeight(mapPos);
            }
            mapPos = viewportCoordToMapCoord(initialVPPos.x, initialVPPos.y, z, viewport->getRotation());
            mapPos.x = std::clamp(mapPos.x, minPosition.x, maxPosition.x);
            mapPos.y = std::clamp(mapPos.y, minPosition.y, maxPosition.y);
        }

        // Determine to which edge the cursor is closest
        [[maybe_unused]] uint32_t closestEdge = 0; // ecx
        const auto xNibble = mapPos.x & 0x1F;
        const auto yNibble = mapPos.y & 0x1F;
        if (xNibble < yNibble)
        {
            if (xNibble + yNibble < 32)
            {
                closestEdge = 0;
            }
            else
            {
                closestEdge = 1;
            }
        }
        else
        {
            if (xNibble + yNibble < 32)
            {
                closestEdge = 3;
            }
            else
            {
                closestEdge = 2;
            }
        }
        return { Pos2(mapPos.x & 0xFFE0, mapPos.y & 0xFFE0) };
    }
}
