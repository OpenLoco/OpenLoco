#include "Company.h"
#include "CompanyManager.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Math/Bound.hpp"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Utility/Numeric.hpp"
#include "Vehicles/Vehicle.h"
#include <algorithm>
#include <array>
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    static void sub_4308D4(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x004308D4, regs);
    }

    static void sub_430971(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00430971, regs);
    }

    static void sub_4309FD(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x004309FD, regs);
    }

    static void sub_430DB6(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00430DB6, regs);
    }

    static void sub_431035(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00431035, regs);
    }

    static void nullsub_3(Company* company)
    {
    }

    static void sub_431104(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00431104, regs);
    }

    static void sub_431193(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00431193, regs);
    }

    static void sub_4311E7(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x004311E7, regs);
    }

    static void nullsub_4(Company* company)
    {
    }

    static void sub_431287(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00431287, regs);
    }

    using UnknownThinkFunction = void (*)(Company*);

    static constexpr UnknownThinkFunction _funcs_430786[] = {
        sub_4308D4,
        sub_430971,
        sub_4309FD,
        sub_430DB6,
        sub_431035,
        nullsub_3,
        sub_431104,
        sub_431193,
        sub_4311E7,
        nullsub_4,
        sub_431287,
    };

    static void sub_431295(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00431295, regs);
    }

    static void sub_43129D(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x0043129D, regs);
    }

    static void sub_4312AF(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x004312AF, regs);
    }

    static void sub_4312BF(Company* company)
    {
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x004312BF, regs);
    }

    static constexpr UnknownThinkFunction _funcs_43079E[] = {
        sub_431295,
        sub_43129D,
        sub_4312AF,
        sub_4312BF,
    };

    static constexpr uint32_t _dword4FE720[] = {
        0x849,
        0x4011,
        0x4051,
        0x808,
        0x20808,
        0x1421,
        0x1120,
        0x98E,
        0x2098E,
        0x98A,
        0x2098A,
        0x21A6,
        0x21A2,
        0x8000,
        0x8082,
        0x10000,
        0x10086,
        0x10082,
        0x80A,
        0x2080A
    };

    bool Company::empty() const
    {
        return name == StringIds::empty;
    }

    // 0x00430762
    void Company::aiThink()
    {
        const auto updatingCompanyId = CompanyManager::updatingCompanyId();

        // Ensure this is only used for Non-Player controlled companies.
        if (CompanyManager::isPlayerCompany(updatingCompanyId))
            return;

        const auto thinkFunc1 = _funcs_430786[var_4A4];
        thinkFunc1(this);

        if (empty())
            return;

        const auto thinkFunc2 = _funcs_43079E[var_4A6];
        thinkFunc2(this);

        if (headquartersX != -1 || (challengeFlags & CompanyFlags::bankrupt) || ((challengeFlags & CompanyFlags::unk0) == 0))
        {
            return;
        }

        // Look for an entry with either town or industry assigned.
        auto index = std::size(var_4A8);
        while (var_4A8[--index].var_00 == 0xFF)
        {
            if (index == 0)
                return;
        }

        auto& entry = var_4A8[index];

        Map::Pos2 pos;
        if ((_dword4FE720[entry.var_00] & 2) != 0)
        {
            auto* industry = IndustryManager::get(static_cast<IndustryId>(entry.var_01));
            pos = { industry->x, industry->y };
        }
        else
        {
            auto* town = TownManager::get(static_cast<TownId>(entry.var_01));
            pos = { town->x, town->y };
        }

        auto& prng = gPrng();
        const auto randPick = prng.randNext();
        // Random tile position 32x32 tiles centered on 0,0 i.e. +-16 tiles
        const auto randPos = Map::Pos2{
            static_cast<coord_t>(randPick & 0x3E0),
            static_cast<coord_t>(Utility::ror<uint32_t>(randPick, 5) & 0x3E0)
        } - Map::TilePos2{ 16, 16 };

        const auto selectedPos = randPos + pos;
        if (Map::validCoords(selectedPos))
        {
            auto tile = Map::TileManager::get(selectedPos);
            auto* surface = tile.surface();

            coord_t z = surface->baseZ() * 4;
            if (surface->slope() != 0)
                z += 16;

            const auto rot = (Utility::ror<uint32_t>(randPick, 10)) & 0x3;
            const auto buildingType = CompanyManager::getHeadquarterBuildingType();

            GameCommands::HeadquarterPlacementArgs args;
            args.pos = Map::Pos3(selectedPos, z);
            args.rotation = rot;
            args.type = buildingType;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
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
                transportTypeCount[enumValue(v->vehicleType)]++;
            }
        }

        Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(companyId));
    }

    // 0x00437FC5
    void Company::updateDaily()
    {
        updateOwnerEmotion();
        for (auto& unk : var_8BB0)
        {
            unk = Math::Bound::sub(unk, 1u);
        }
        updateDailyLogic();
        var_8BC4 = Math::Bound::sub(var_8BC4, 1u);
        if (jailStatus != 0)
        {
            jailStatus = Math::Bound::sub(jailStatus, 1u);
            if (jailStatus == 0)
            {
                Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(id()));
                Ui::WindowManager::invalidate(Ui::WindowType::news);
                Ui::WindowManager::invalidate(Ui::WindowType(0x2E));
            }
        }
        if (CompanyManager::isPlayerCompany(id()))
        {
            updateDailyPlayer();
        }
        if (CompanyManager::getControllingId() == id())
        {
            updateDailyControllingPlayer();
        }
    }

    // 0x00438205
    void Company::updateDailyLogic()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        regs.ebx = enumValue(id());
        call(0x00438205, regs);
    }

    // 0x004387D0
    void Company::updateDailyPlayer()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        regs.ebx = enumValue(id());
        call(0x004387D0, regs);
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

        return (unlockedVehicles[vehicleTypeIndex] & (1 << (vehicleIndex & 0x1F))) != 0;
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

    void Company::updateDailyControllingPlayer()
    {
        updateLoanAutorepay();
    }

    void Company::updateLoanAutorepay()
    {
        if (currentLoan > 0 && cash > 0 && ((challengeFlags & CompanyFlags::autopayLoan) != 0))
        {
            GameCommands::ChangeLoanArgs args{};
            args.newLoan = currentLoan - std::max<currency32_t>(0, std::min<currency32_t>(currentLoan, cash.asInt64()));

            GameCommands::setUpdatingCompanyId(id());
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }

    // 0x004B8ED2
    void Company::updateVehicleColours()
    {
        for (auto v : EntityManager::VehicleList())
        {
            if (v->owner != id())
            {
                continue;
            }
            Vehicles::Vehicle train(v);
            for (auto& car : train.cars)
            {
                const auto* vehObject = car.body->getObject();
                auto colour = mainColours;
                if (customVehicleColoursSet & (1 << vehObject->colour_type))
                {
                    colour = vehicleColours[vehObject->colour_type - 1];
                }
                for (auto& carComponent : car)
                {
                    carComponent.front->colourScheme = colour;
                    carComponent.back->colourScheme = colour;
                    carComponent.body->colourScheme = colour;
                }
            }
        }
        Gfx::invalidateScreen();
    }

    // 0x0042F0C1
    static void updateHeadquartersColourAtTile(const Map::TilePos2& pos, uint8_t zPos, Colour_t newColour)
    {
        auto tile = Map::TileManager::get(pos);
        for (auto& element : tile)
        {
            if (element.baseZ() != zPos)
                continue;

            auto* building = element.as<Map::BuildingElement>();
            if (building == nullptr)
                continue;

            building->setColour(newColour);
            return;
        }
    }

    // 0x0042F07B
    void Company::updateHeadquartersColour()
    {
        if (headquartersX == -1)
            return;

        Colour_t colour = mainColours.primary;
        auto hqPos = Map::TilePos2(Map::Pos2(headquartersX, headquartersY));
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(0, 0), headquartersZ, colour);
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(1, 0), headquartersZ, colour);
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(1, 1), headquartersZ, colour);
        updateHeadquartersColourAtTile(hqPos + Map::TilePos2(0, 1), headquartersZ, colour);
    }

    // 0x00437F47
    void Company::updateOwnerEmotion()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        regs.ebx = enumValue(id());
        call(0x00437F47, regs);
    }
}
