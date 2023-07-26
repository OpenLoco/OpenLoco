#include "CompanyAi.h"
#include "Company.h"
#include "CompanyManager.h"
#include "Date.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/GameCommands.h"
#include "Industry.h"
#include "IndustryManager.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Random.h"
#include "Station.h"
#include "StationManager.h"
#include "TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x00494805
    static void sub_494805(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00494805, regs);
    }

    // 0x004308D4
    static void sub_4308D4(Company& company)
    {
        company.var_85F6++;
        if (company.var_85F6 < 672)
        {
            company.var_4A4 = 2;
            company.var_4A5 = 0;
            return;
        }

        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            bool hasAssets = false;
            for (auto& unk : company.var_4A8)
            {
                if (unk.var_00 == 0xFF)
                {
                    hasAssets = true;
                    break;
                }
            }
            if (!hasAssets)
            {
                for (auto& station : StationManager::stations())
                {
                    if (station.owner == company.id())
                    {
                        hasAssets = true;
                        break;
                    }
                }
            }
            if (!hasAssets)
            {
                company.var_4A4 = 10;
                company.var_85C4 = 0;
                return;
            }
        }

        company.var_85F6 = 0;
        company.var_4A4 = 1;
        company.var_2578 = 0xFF;
        sub_494805(company);
    }

    // 0x00487F8D
    static bool sub_487F8D(const Company& company, const Company::unk4A8& unk)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return true;
        }
        if (unk.var_88 < 3)
        {
            return false;
        }
        // 27 / 8 ???
        const auto val = unk.var_7C * 3;
        const auto val2 = val + (val / 8);
        return unk.var_84 >= val2;
    }

    // 0x00488050
    static bool sub_488050(const Company& company, const Company::unk4A8& unk)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&unk);
        return call(0x00488050, regs) & X86_FLAG_CARRY;
    }

    // 0x00430971
    static void sub_430971(Company& company)
    {
        company.var_2578++;
        if (company.var_2578 < 60)
        {
            const auto& unk = company.var_4A8[company.var_2578];
            if (unk.var_00 == 0xFF)
            {
                sub_430971(company);
                return;
            }

            if (sub_487F8D(company, unk))
            {
                company.var_4A4 = 7;
                company.var_4A5 = 0;
                StationManager::sub_437F29(company.id(), 8);
                return;
            }

            if (sub_488050(company, unk))
            {
                company.var_4A4 = 8;
                company.var_4A5 = 0;
            }
            return;
        }
        if (((company.challengeFlags & CompanyFlags::unk0) != CompanyFlags::none)
            || (getCurrentDay() - company.startedDate <= 42))
        {
            company.var_4A4 = 2;
            company.var_4A5 = 0;
            return;
        }
        CompanyManager::aiDestroy(company.id());
    }

    // 0x004309FD
    static void sub_4309FD(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x004309FD, regs);
    }

    // 0x00430DB6
    static void sub_430DB6(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00430DB6, regs);
    }

    // 0x00431035
    static void sub_431035(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00431035, regs);
    }

    static void nullsub_3([[maybe_unused]] Company& company)
    {
    }

    // 0x00431104
    static void sub_431104(Company& company)
    {
        // try sell a vehicle?
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00431104, regs);
    }

    // 0x00431193
    static void sub_431193(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00431193, regs);
    }

    // 0x00431209
    static void sub_431209(Company& company, Company::unk4A8& unk)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&unk);
        call(0x00431209, regs);
    }

    // 0x00431216
    static void sub_431216(Company& company, Company::unk4A8& unk)
    {
        // branch on sub_487E6D (which is a nop) would have made var_4A4 = 1
        company.var_4A5 = 2;
    }

    // 0x0043122D
    static void sub_43122D(Company& company, Company::unk4A8& unk)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&unk);
        call(0x0043122D, regs);
    }

    // 0x00431244
    static void sub_431244(Company& company, Company::unk4A8& unk)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&unk);
        call(0x00431244, regs);
    }

    // 0x00431254
    static void sub_431254(Company& company, Company::unk4A8& unk)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&unk);
        call(0x00431254, regs);
    }

    // 0x00431279
    static void sub_431279(Company& company, Company::unk4A8& unk)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&unk);
        call(0x00431279, regs);
    }

    using Unk4311E7ThinkFunction = void (*)(Company&, Company::unk4A8&);

    static constexpr std::array<Unk4311E7ThinkFunction, 6> _funcs_4F9530 = {
        sub_431209,
        sub_431216,
        sub_43122D,
        sub_431244,
        sub_431254,
        sub_431279,
    };

    // 0x004311E7
    static void sub_4311E7(Company& company)
    {
        _funcs_4F9530[company.var_4A5](company, company.var_4A8[company.var_2578]);
    }

    static void nullsub_4([[maybe_unused]] Company& company)
    {
    }

    // 0x00431287
    static void sub_431287(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00431287, regs);
    }

    using UnknownThinkFunction = void (*)(Company&);

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

    // 0x00431295
    static void sub_431295(Company& company)
    {
        company.var_4A6 = 1;
    }

    // 0x0043129D
    static void sub_43129D(Company& company)
    {
        company.var_4A6 = 2;
        company.var_259E = 0;
    }

    // 0x00487784
    static bool tryPlaceVehicles(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        return call(0x00487784, regs) & X86_FLAG_CARRY;
    }

    // 0x004312AF
    static void sub_4312AF(Company& company)
    {
        if (tryPlaceVehicles(company))
        {
            company.var_4A6 = 3;
        }
    }

    // 0x004312BF
    static void sub_4312BF(Company& company)
    {
        company.var_4A6 = 0;
    }

    static void callThinkFunc2(Company& company)
    {
        switch (company.var_4A6)
        {
            case 0:
                sub_431295(company);
                break;
            case 1:
                sub_43129D(company);
                break;
            case 2:
                sub_4312AF(company);
                break;
            case 3:
                sub_4312BF(company);
                break;
            default:
                assert(false);
                return;
        }
    }

    // 0x00430762
    void aiThink(const CompanyId id)
    {
        // const auto updatingCompanyId = CompanyManager::getUpdatingCompanyId();

        // Ensure this is only used for Non-Player controlled companies.
        if (CompanyManager::isPlayerCompany(id))
            return;

        auto* company = CompanyManager::get(id);

        const auto thinkFunc1 = _funcs_430786[company->var_4A4];
        thinkFunc1(*company);

        if (company->empty())
            return;

        callThinkFunc2(*company);

        if (company->headquartersX != -1 || (company->challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none || (company->challengeFlags & CompanyFlags::unk0) == CompanyFlags::none)
        {
            return;
        }

        // Look for an entry with either town or industry assigned.
        auto index = std::size(company->var_4A8);
        while (company->var_4A8[--index].var_00 == 0xFF)
        {
            if (index == 0)
                return;
        }

        auto& entry = company->var_4A8[index];

        World::Pos2 pos;
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

        auto& prng = gPrng1();
        const auto randPick = prng.randNext();
        // Random tile position 32x32 tiles centered on 0,0 i.e. +-16 tiles
        const auto randPos = World::Pos2{
            static_cast<coord_t>(randPick & 0x3E0),
            static_cast<coord_t>(Numerics::ror<uint32_t>(randPick, 5) & 0x3E0)
        } - World::toWorldSpace(World::TilePos2{ 16, 16 });

        const auto selectedPos = randPos + pos;
        if (World::validCoords(selectedPos))
        {
            auto tile = World::TileManager::get(selectedPos);
            auto* surface = tile.surface();

            coord_t z = surface->baseHeight();
            if (surface->slope() != 0)
                z += 16;

            const auto rot = (Numerics::ror<uint32_t>(randPick, 10)) & 0x3;
            const auto buildingType = CompanyManager::getHeadquarterBuildingType();

            GameCommands::HeadquarterPlacementArgs args;
            args.pos = World::Pos3(selectedPos, z);
            args.rotation = rot;
            args.type = buildingType;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }
}
