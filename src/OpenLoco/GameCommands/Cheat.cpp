#include "Cheat.h"
#include "../CompanyManager.h"
#include "../Console.h"
#include "../Economy/Currency.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../MessageManager.h"
#include "../Scenario.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Types.hpp"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::GameCommands
{
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;

    namespace Cheats
    {
        static uint32_t acquireAssets(CompanyId targetCompanyId)
        {
            auto ourCompanyId = CompanyManager::getUpdatingCompanyId();

            // First phase: change ownership of all tile elements that currently belong to the target company.
            for (auto& element : TileManager::getElements())
            {
                auto* roadElement = element.as<RoadElement>();
                if (roadElement != nullptr)
                {
                    roadElement->setOwner(ourCompanyId);
                    continue;
                }

                auto* trackElement = element.as<TrackElement>();
                if (trackElement != nullptr)
                {
                    trackElement->setOwner(ourCompanyId);
                    continue;
                }
            }

            // Second phase: change ownership of all stations that currently belong to the target company.
            for (auto& station : StationManager::stations())
            {
                if (station.owner != targetCompanyId)
                    continue;

                station.owner = ourCompanyId;
            }

            // Third phase: change ownership of all vehicles that currently belong to the target company.
            for (auto vehicle : EntityManager::VehicleList())
            {
                if (vehicle->owner != targetCompanyId)
                    continue;

                Vehicles::Vehicle train(*vehicle);
                train.head->owner = ourCompanyId;
                train.veh1->owner = ourCompanyId;
                train.veh2->owner = ourCompanyId;
                train.tail->owner = ourCompanyId;

                for (auto& car : train.cars)
                {
                    for (auto& component : car)
                    {
                        component.front->owner = ourCompanyId;
                        component.body->owner = ourCompanyId;
                        component.back->owner = ourCompanyId;
                    }
                }
            }

            return 0;
        }

        static uint32_t addCash(currency32_t amount)
        {
            auto company = CompanyManager::getPlayerCompany();
            company->cash = company->cash + amount;
            return 0;
        }

        static uint32_t clearLoan()
        {
            auto company = CompanyManager::getPlayerCompany();
            company->currentLoan = 0;
            return 0;
        }

        static uint32_t companyRatings(bool absolute, int32_t value)
        {
            auto companyId = CompanyManager::getControllingId();

            for (auto& town : TownManager::towns())
            {
                // Does this town have a rating for our company?
                if (!(town.companies_with_rating &= (1 << enumValue(companyId))))
                    continue;

                int16_t newRanking{};
                if (absolute)
                {
                    newRanking = value * kMaxCompanyRating;
                }
                else
                {
                    newRanking = town.company_ratings[enumValue(companyId)] + kMaxCompanyRating;
                    newRanking *= 1.0f + (1.0f / value);
                    newRanking -= kMaxCompanyRating;
                }

                // Set the new rating.
                town.company_ratings[enumValue(companyId)] = std::clamp<int16_t>(newRanking, kMinCompanyRating, kMaxCompanyRating);
            }

            return 0;
        }

        static uint32_t switchCompany(CompanyId targetCompanyId)
        {
            auto ourId = CompanyManager::getControllingId();
            auto otherId = CompanyManager::getSecondaryPlayerId();

            // Already controlling the target company?
            if (targetCompanyId == ourId)
                return 0;

            // Is the other player controlling the target company? Swap companies.
            if (targetCompanyId == otherId)
            {
                CompanyManager::setSecondaryPlayerId(ourId);
                CompanyManager::setControllingId(otherId);
                return 0;
            }

            // Change control over to the other company.
            CompanyManager::setControllingId(targetCompanyId);
            return 0;
        }

        static uint32_t toggleBankruptcy(CompanyId targetCompanyId)
        {
            auto company = CompanyManager::get(targetCompanyId);
            company->challengeFlags ^= CompanyFlags::bankrupt;
            return 0;
        }

        static uint32_t toggleJail(CompanyId targetCompanyId)
        {
            auto company = CompanyManager::get(targetCompanyId);
            company->jailStatus = 30;
            return 0;
        }

        static uint32_t vehicleReliability(int32_t newReliablity)
        {
            auto ourCompanyId = CompanyManager::getUpdatingCompanyId();

            for (auto vehicle : EntityManager::VehicleList())
            {
                if (vehicle->owner != ourCompanyId)
                    continue;

                Vehicles::Vehicle train(*vehicle);
                train.veh2->reliability = newReliablity;

                // Set reliability for the front bogie component on each car.
                for (auto& car : train.cars)
                {
                    car.front->reliability = newReliablity * 256;
                }
            }
            return 0;
        }

        static uint32_t modifyDateCheat(int32_t year, int32_t month, int32_t day)
        {
            OpenLoco::Scenario::initialiseDate(static_cast<uint16_t>(year), static_cast<MonthId>(month), static_cast<uint8_t>(day));
            Console::log("Date set to: Day=%u Month=%u Year=%u", day, month, year);
            return 0;
        }
    }

    static uint32_t cheat(CheatCommand command, int32_t param1, int32_t param2, int32_t param3)
    {
        switch (command)
        {
            case CheatCommand::acquireAssets:
                return Cheats::acquireAssets(CompanyId(param1));

            case CheatCommand::addCash:
                return Cheats::addCash(param1);

            case CheatCommand::clearLoan:
                return Cheats::clearLoan();

            case CheatCommand::companyRatings:
                return Cheats::companyRatings(param1, param2);

            case CheatCommand::switchCompany:
                return Cheats::switchCompany(CompanyId(param1));

            case CheatCommand::toggleBankruptcy:
                return Cheats::toggleBankruptcy(CompanyId(param1));

            case CheatCommand::toggleJail:
                return Cheats::toggleJail(CompanyId(param1));

            case CheatCommand::vehicleReliability:
                return Cheats::vehicleReliability(param1);

            case CheatCommand::modifyDate:
                return Cheats::modifyDateCheat(param1, param2, param3);

            default:
                break;
        }

        return 0;
    }

    void cheat(registers& regs)
    {
        regs.ebx = cheat(CheatCommand(regs.eax), regs.ebx, regs.ecx, regs.edx);
    }

    // 0x004BAC53
    static uint32_t vehicleShuntCheat(EntityId head, uint8_t flags)
    {
        auto* veh = EntityManager::get<Vehicles::VehicleHead>(head);
        if (veh == nullptr)
        {
            return GameCommands::FAILURE;
        }
        if (flags & Flags::apply)
        {
            veh->var_0C |= Vehicles::Flags0C::shuntCheat;
        }
        return 0;
    }

    void vehicleShuntCheat(registers& regs)
    {
        VehicleApplyShuntCheatArgs args(regs);
        regs.ebx = vehicleShuntCheat(args.head, regs.bl);
    }

    // 0x00438A08
    static uint32_t freeCashCheat(uint8_t flags)
    {
        if (flags & Flags::apply)
        {
            auto* company = CompanyManager::get(_updatingCompanyId);
            company->jailStatus = 30;
            Ui::WindowManager::invalidate(Ui::WindowType::company, static_cast<Ui::WindowNumber_t>(*_updatingCompanyId));
            Ui::WindowManager::invalidate(Ui::WindowType::news);
            Ui::WindowManager::invalidate(static_cast<Ui::WindowType>(0x2E));
            MessageManager::post(MessageType::companyCheated, CompanyId::null, static_cast<uint16_t>(*_updatingCompanyId), 0xFFFF);
            StationManager::sub_437F29(_updatingCompanyId, 4);
        }
        return 0;
    }

    void freeCashCheat(registers& regs)
    {
        regs.ebx = freeCashCheat(regs.bl);
    }
}
