#include "Company.h"
#include "interop/interop.hpp"
#include "localisation/FormatArguments.hpp"
#include "localisation/string_ids.h"
#include "things/thingmgr.h"
#include "ui/WindowManager.h"
#include <algorithm>
#include <array>
#include <map>

using namespace openloco::interop;

namespace openloco
{
    static loco_global<company_id_t[2], 0x00525E3C> _player_company[2];

    bool isPlayerCompany(company_id_t id)
    {
        auto& player_company = *((std::array<company_id_t, 2>*)_player_company->get());
        auto findResult = std::find(
            player_company.begin(),
            player_company.end(),
            id);
        return findResult != player_company.end();
    }

    company_id_t company::id() const
    {
        auto first = (company*)0x00531784;
        return (company_id_t)(this - first);
    }

    bool company::empty() const
    {
        return name == string_ids::empty;
    }

    // 0x00430762
    void company::aiThink()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x00430762, regs);
    }

    // 0x00437ED0
    void company::recalculateTransportCounts()
    {
        // Reset all counts to 0
        for (auto& count : transportTypeCount)
        {
            count = 0;
        }

        auto companyId = id();
        for (auto v : thingmgr::VehicleList())
        {
            if (v->owner == companyId)
            {
                transportTypeCount[static_cast<uint8_t>(v->vehicleType)]++;
            }
        }

        ui::WindowManager::invalidate(ui::WindowType::company, companyId);
    }

    // Converts performance index to rating
    // 0x437D60
    constexpr CorporateRating performanceToRating(int16_t performanceIndex)
    {
        return static_cast<CorporateRating>(std::min(9, performanceIndex / 100));
    }

    static std::map<CorporateRating, string_id> _ratingNames = {
        { CorporateRating::platelayer, string_ids::corporate_rating_platelayer },
        { CorporateRating::engineer, string_ids::corporate_rating_engineer },
        { CorporateRating::trafficManager, string_ids::corporate_rating_traffic_manager },
        { CorporateRating::transportCoordinator, string_ids::corporate_rating_transport_coordinator },
        { CorporateRating::routeSupervisor, string_ids::corporate_rating_route_supervisor },
        { CorporateRating::director, string_ids::corporate_rating_director },
        { CorporateRating::chiefExecutive, string_ids::corporate_rating_chief_executive },
        { CorporateRating::chairman, string_ids::corporate_rating_chairman },
        { CorporateRating::president, string_ids::corporate_rating_president },
        { CorporateRating::tycoon, string_ids::corporate_rating_tycoon },
    };

    static string_id getCorporateRatingAsStringId(CorporateRating rating)
    {
        auto it = _ratingNames.find(rating);
        if (it != _ratingNames.end())
        {
            return it->second;
        }
        return string_ids::corporate_rating_platelayer;
    }

    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args)
    {
        args.push(performanceIndex);
        args.push(getCorporateRatingAsStringId(performanceToRating(performanceIndex)));
    }

    bool company::isVehicleIndexUnlocked(const uint8_t vehicleIndex) const
    {
        auto vehicleTypeIndex = vehicleIndex >> 5;

        return (unlocked_vehicles[vehicleTypeIndex] & (1 << (vehicleIndex & 0x1F))) != 0;
    }
}
