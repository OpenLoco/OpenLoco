#include "Company.h"
#include "Entities/EntityManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <algorithm>
#include <array>
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    static loco_global<CompanyId_t[2], 0x00525E3C> _playerCompanies;

    bool isPlayerCompany(CompanyId_t id)
    {
        auto findResult = std::find(
            _playerCompanies.begin(),
            _playerCompanies.end(),
            id);
        return findResult != _playerCompanies.end();
    }

    CompanyId_t Company::id() const
    {
        auto first = (Company*)0x00531784;
        return (CompanyId_t)(this - first);
    }

    bool Company::empty() const
    {
        return name == StringIds::empty;
    }

    // 0x00430762
    void Company::aiThink()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x00430762, regs);
    }

    // 0x00437ED0
    void Company::recalculateTransportCounts()
    {
        // Reset all counts to 0
        for (auto& count : transportTypeCount)
        {
            count = 0;
        }

        auto companyId = id();
        for (auto v : EntityManager::VehicleList())
        {
            if (v->owner == companyId)
            {
                transportTypeCount[static_cast<uint8_t>(v->vehicleType)]++;
            }
        }

        Ui::WindowManager::invalidate(Ui::WindowType::company, companyId);
    }

    // Converts performance index to rating
    // 0x00437D60
    // input:
    // ax = performanceIndex
    // output:
    // ax = return value, coprorate rating
    constexpr CorporateRating performanceToRating(int16_t performanceIndex)
    {
        return static_cast<CorporateRating>(std::min(9, performanceIndex / 100));
    }

    static std::map<CorporateRating, string_id> _ratingNames = {
        { CorporateRating::platelayer, StringIds::corporate_rating_platelayer },
        { CorporateRating::engineer, StringIds::corporate_rating_engineer },
        { CorporateRating::trafficManager, StringIds::corporate_rating_traffic_manager },
        { CorporateRating::transportCoordinator, StringIds::corporate_rating_transport_coordinator },
        { CorporateRating::routeSupervisor, StringIds::corporate_rating_route_supervisor },
        { CorporateRating::director, StringIds::corporate_rating_director },
        { CorporateRating::chiefExecutive, StringIds::corporate_rating_chief_executive },
        { CorporateRating::chairman, StringIds::corporate_rating_chairman },
        { CorporateRating::president, StringIds::corporate_rating_president },
        { CorporateRating::tycoon, StringIds::corporate_rating_tycoon },
    };

    static string_id getCorporateRatingAsStringId(CorporateRating rating)
    {
        auto it = _ratingNames.find(rating);
        if (it != _ratingNames.end())
        {
            return it->second;
        }
        return StringIds::corporate_rating_platelayer;
    }

    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args)
    {
        args.push(performanceIndex);
        args.push(getCorporateRatingAsStringId(performanceToRating(performanceIndex)));
    }

    bool Company::isVehicleIndexUnlocked(const uint8_t vehicleIndex) const
    {
        auto vehicleTypeIndex = vehicleIndex >> 5;

        return (unlocked_vehicles[vehicleTypeIndex] & (1 << (vehicleIndex & 0x1F))) != 0;
    }

    // 0x00487FCC
    void Company::updateQuarterly()
    {
        for (auto& unk : var_4A8)
        {
            if (unk.var_00 == 0xFF)
                continue;

            unk.var_88 = std::min(0xFF, unk.var_88 + 1);
            unk.var_84 = unk.var_80;
            unk.var_80 = 0;
            currency32_t totalRunCost = 0;
            for (auto i = 0; i < unk.var_44; ++i)
            {
                auto* vehHead = EntityManager::get<Vehicles::VehicleHead>(unk.var_66[i]);
                totalRunCost += vehHead->calculateRunningCost();
            }
            unk.var_7C = totalRunCost;
        }
    }

    // 0x004B8ED2
    void Company::updateVehicleColours()
    {
        registers regs;
        regs.dl = id();
        call(0x004B8ED2, regs);
    }

    // 0x0042F0C1
    static void updateHeadquartersColourAtTile(const Map::TilePos2& pos, uint8_t zPos, Colour_t newColour)
    {
        auto tile = Map::TileManager::get(pos);
        for (auto& element : tile)
        {
            if (element.baseZ() != zPos)
                continue;

            auto building = element.asBuilding();
            if (building == nullptr)
                continue;

            building->setColour(newColour);
            return;
        }
    }

    // 0x0042F07B
    void Company::updateHeadquartersColour()
    {
        if (headquarters_x == -1)
            return;

        Colour_t colour = mainColours.primary;
        auto hqPos = Map::TilePos2(Map::Pos2(headquarters_x, headquarters_y));
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(0, 0), headquarters_z, colour);
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(1, 0), headquarters_z, colour);
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(1, 1), headquarters_z, colour);
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(0, 1), headquarters_z, colour);
    }
}
