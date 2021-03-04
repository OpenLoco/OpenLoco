#include "CompanyManager.h"
#include "Config.h"
#include "GameCommands.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Map/Tile.h"
#include "OpenLoco.h"
#include "Ptr.h"
#include "Things/ThingManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::CompanyManager
{
    static loco_global<company_id_t[2], 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00525FCB> _byte_525FCB;
    static loco_global<uint8_t, 0x00526214> _company_competition_delay;
    static loco_global<uint8_t, 0x00525FB7> _company_max_competing;
    static loco_global<uint8_t, 0x00525E3C> _byte_525E3C;
    static loco_global<uint8_t, 0x00525E3D> _byte_525E3D;
    static loco_global<company[max_companies], 0x00531784> _companies;
    static loco_global<uint8_t[max_companies + 1], 0x009C645C> _company_colours;
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;

    static void produceCompanies();

    // 0x0042F7F8
    void reset()
    {
        call(0x0042F7F8);
    }

    company_id_t updatingCompanyId()
    {
        return _updating_company_id;
    }

    void updatingCompanyId(company_id_t id)
    {
        _updating_company_id = id;
    }

    std::array<company, max_companies>& companies()
    {
        auto arr = (std::array<company, max_companies>*)_companies.get();
        return *arr;
    }

    company* get(company_id_t id)
    {
        auto index = id;
        if (index < _companies.size())
        {
            return &_companies[index];
        }
        return nullptr;
    }

    company_id_t getControllingId()
    {
        return _player_company[0];
    }

    company* getPlayerCompany()
    {
        return &_companies[_player_company[0]];
    }

    uint8_t getCompanyColour(company_id_t id)
    {
        return _company_colours[id];
    }

    uint8_t getPlayerCompanyColour()
    {
        return _company_colours[_player_company[0]];
    }

    // 0x00430319
    void update()
    {
        if (!isEditorMode() && !Config::getNew().companyAIDisabled)
        {
            company_id_t id = scenarioTicks() & 0x0F;
            auto company = get(id);
            if (company != nullptr && !isPlayerCompany(id) && !company->empty())
            {
                updatingCompanyId(id);
                company->aiThink();
            }

            _byte_525FCB++;
            if (_byte_525FCB >= 192)
            {
                _byte_525FCB = 0;
                produceCompanies();
            }
        }
    }

    static void sub_42F9AC()
    {
        call(0x0042F9AC);
    }

    // 0x0042FDE2
    void determineAvailableVehicles()
    {
        for (auto& company : _companies)
        {
            if (company.empty())
                continue;

            VehicleManager::determineAvailableVehicles(company);
        }
    }

    // 0x004306D1
    static void produceCompanies()
    {
        if (_company_competition_delay == 0 && _company_max_competing != 0)
        {
            int32_t companies_active = 0;
            for (const auto& company : companies())
            {
                auto id = company.id();
                if (!company.empty() && id != _byte_525E3C && id != _byte_525E3D)
                {
                    companies_active++;
                }
            }

            auto& prng = gPrng();

            if (prng.randNext(16) == 0)
            {
                if (prng.randNext(_company_max_competing) + 1 > companies_active)
                {
                    // Creates new company.
                    sub_42F9AC();
                }
            }
        }
    }

    company* getOpponent()
    {
        return &_companies[_player_company[1]];
    }

    // 0x00438047
    // Returns a string between 1810 and 1816 with up to two arguments.
    string_id getOwnerStatus(company_id_t id, FormatArguments& args)
    {
        registers regs;
        regs.esi = ToInt(get(id));
        call(0x00438047, regs);

        args.push(regs.ecx);
        args.push(regs.edx);
        return regs.bx;
    }

    owner_status getOwnerStatus(company_id_t id)
    {
        registers regs;
        regs.esi = ToInt(get(id));
        call(0x00438047, regs);

        owner_status ownerStatus;

        ownerStatus.string = regs.bx;
        ownerStatus.argument1 = regs.ecx;
        ownerStatus.argument2 = regs.edx;

        return ownerStatus;
    }

    // 0x004383ED
    void updateOwnerStatus()
    {
        if (OpenLoco::isTitleMode() || OpenLoco::isEditorMode())
        {
            return;
        }

        auto company = CompanyManager::get(_updating_company_id);
        if (company == nullptr)
        {
            return;
        }

        company->update_counter += 1;
        if ((company->update_counter % 128) != 0)
            return;

        for (size_t i = 0; i < WindowManager::count(); i++)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            auto vehicle = ThingManager::get<Vehicles::VehicleBase>(w->number);
            if (vehicle->x == Location::null)
                continue;

            if (vehicle->getOwner() != _updating_company_id)
                continue;

            GameCommands::do_73(vehicle->id);
            return;
        }

        auto main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        auto viewport = main->viewports[0];
        if (viewport == nullptr)
            return;

        Gfx::point_t screenPosition;
        screenPosition.x = viewport->x + viewport->width / 2;
        screenPosition.y = viewport->y + viewport->height / 2;

        registers r1;
        r1.ax = screenPosition.x;
        r1.bx = screenPosition.y;
        call(0x0045F1A7, r1);
        Ui::viewport* vp = ToPtr(Ui::viewport, r1.edi);
        auto mapPosition = Map::map_pos(r1.ax, r1.bx);

        // Happens if center of viewport is obstructed. Probably estimates the centre location
        if (mapPosition.x == Location::null || viewport != vp)
        {
            mapPosition = viewport->getCentreMapPosition();
        }

        GameCommands::do_73(mapPosition);
    }

}
