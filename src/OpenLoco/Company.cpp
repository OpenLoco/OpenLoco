#include "Company.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Ptr.h"
#include "Things/ThingManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <algorithm>
#include <array>
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco
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
        return name == StringIds::empty;
    }

    // 0x00430762
    void company::aiThink()
    {
        registers regs;
        regs.esi = ToInt(this);
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
        for (auto v : ThingManager::VehicleList())
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

    bool company::isVehicleIndexUnlocked(const uint8_t vehicleIndex) const
    {
        auto vehicleTypeIndex = vehicleIndex >> 5;

        return (unlocked_vehicles[vehicleTypeIndex] & (1 << (vehicleIndex & 0x1F))) != 0;
    }
}
