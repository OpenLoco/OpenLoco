#include "CompanyManager.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Entities/Misc.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Objects/AirportObject.h"
#include "Objects/DockObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "OpenLoco.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::CompanyManager
{
    static loco_global<uint8_t, 0x00525FCB> _byte_525FCB;
    static loco_global<uint8_t, 0x00526214> _company_competition_delay;
    static loco_global<uint8_t, 0x00525FB7> _company_max_competing;
    static loco_global<uint8_t[Limits::maxCompanies + 1], 0x009C645C> _company_colours;
    static loco_global<CompanyId_t, 0x009C68EB> _updating_company_id;

    static void produceCompanies();

    static auto& rawCompanies()
    {
        return getGameState().companies;
    }

    static auto& rawPlayerCompanies() { return getGameState().playerCompanies; }

    // 0x0042F7F8
    void reset()
    {
        // First, empty all non-empty companies.
        for (auto& company : companies())
            company.name = StringIds::empty;

        _byte_525FCB = 0;

        // Reset player companies depending on network mode.
        if (isNetworkHost())
        {
            rawPlayerCompanies()[0] = 1;
            rawPlayerCompanies()[1] = 0;
        }
        else if (isNetworked())
        {
            rawPlayerCompanies()[0] = 0;
            rawPlayerCompanies()[1] = 1;
        }
        else
        {
            rawPlayerCompanies()[0] = 0;
            rawPlayerCompanies()[1] = CompanyId::null;
        }

        // Reset primary company colours.
        rawCompanies()[0].mainColours.primary = Colour::mutedGreen1;
        updateColours();
    }

    CompanyId_t updatingCompanyId()
    {
        return _updating_company_id;
    }

    void updatingCompanyId(CompanyId_t id)
    {
        _updating_company_id = id;
    }

    FixedVector<Company, Limits::maxCompanies> companies()
    {
        return FixedVector(rawCompanies());
    }

    Company* get(CompanyId_t id)
    {
        auto index = id;
        if (index < Limits::maxCompanies)
        {
            return &rawCompanies()[index];
        }
        return nullptr;
    }

    CompanyId_t getControllingId()
    {
        return rawPlayerCompanies()[0];
    }

    CompanyId_t getSecondaryPlayerId()
    {
        return rawPlayerCompanies()[1];
    }

    void setControllingId(CompanyId_t id)
    {
        rawPlayerCompanies()[0] = id;
    }

    void setSecondaryPlayerId(CompanyId_t id)
    {
        rawPlayerCompanies()[1] = id;
    }

    Company* getPlayerCompany()
    {
        return get(rawPlayerCompanies()[0]);
    }

    uint8_t getCompanyColour(CompanyId_t id)
    {
        return _company_colours[id];
    }

    uint8_t getPlayerCompanyColour()
    {
        return _company_colours[rawPlayerCompanies()[0]];
    }

    bool isPlayerCompany(CompanyId_t id)
    {
        auto findResult = std::find(
            std::begin(rawPlayerCompanies()),
            std::end(rawPlayerCompanies()),
            id);
        return findResult != std::end(rawPlayerCompanies());
    }

    // 0x00430319
    void update()
    {
        if (!isEditorMode() && !Config::getNew().companyAIDisabled)
        {
            CompanyId_t id = scenarioTicks() & 0x0F;
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

    // 0x00487FC1
    void updateQuarterly()
    {
        for (auto& company : companies())
        {
            company.updateQuarterly();
        }
    }

    static void sub_42F9AC()
    {
        call(0x0042F9AC);
    }

    // 0x0042F23C
    currency32_t calculateDeliveredCargoPayment(uint8_t cargoItem, int32_t numUnits, int32_t distance, uint16_t numDays)
    {
        registers regs;
        regs.eax = cargoItem;
        regs.ebx = numUnits;
        regs.ecx = distance;
        regs.edx = numDays;
        call(0x0042F23C, regs);
        return regs.eax;
    }

    // 0x0042FDE2
    void determineAvailableVehicles()
    {
        for (auto& company : companies())
        {
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
                if (!isPlayerCompany(company.id()))
                    companies_active++;
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

    Company* getOpponent()
    {
        return get(rawPlayerCompanies()[1]);
    }

    // 0x00438047
    // Returns a string between 1810 and 1816 with up to two arguments.
    string_id getOwnerStatus(CompanyId_t id, FormatArguments& args)
    {
        auto* company = get(id);
        if (company == nullptr)
        {
            return StringIds::company_status_empty;
        }

        if (company->challenge_flags & CompanyFlags::bankrupt)
            return StringIds::company_status_bankrupt;

        const string_id observationStatusStrings[] = {
            StringIds::company_status_empty,
            StringIds::company_status_building_track_road,
            StringIds::company_status_building_airport,
            StringIds::company_status_building_dock,
            StringIds::company_status_checking_services,
            StringIds::company_status_surveying_landscape,
        };

        string_id statusString = observationStatusStrings[company->observationStatus];
        if (company->observationStatus == ObservationStatus::empty || company->observationTownId == TownId::null)
            return StringIds::company_status_empty;

        switch (company->observationStatus)
        {
            case ObservationStatus::buildingTrackRoad:
                if (company->observationObject & 0x80)
                {
                    auto* obj = ObjectManager::get<RoadObject>(company->observationObject & 0xFF7F);
                    if (obj != nullptr)
                        args.push(obj->name);
                }
                else
                {
                    auto* obj = ObjectManager::get<TrackObject>(company->observationObject);
                    if (obj != nullptr)
                        args.push(obj->name);
                }
                break;

            case ObservationStatus::buildingAirport:
            {
                auto* obj = ObjectManager::get<AirportObject>(company->observationObject);
                if (obj != nullptr)
                    args.push(obj->name);
                break;
            }

            case ObservationStatus::buildingDock:
            {
                auto* obj = ObjectManager::get<DockObject>(company->observationObject);
                if (obj != nullptr)
                    args.push(obj->name);
                break;
            }

            default:
                break;
        }

        auto* town = TownManager::get(company->observationTownId);
        args.push(town->name);

        return statusString;
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

            auto vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(w->number));
            if (vehicle->position.x == Location::null)
                continue;

            if (vehicle->owner != _updating_company_id)
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

        auto screenPosition = viewport->getUiCentre();

        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi(screenPosition);

        Map::Pos2 mapPosition{};
        if (!res || res->second != viewport)
        {
            // Happens if center of viewport is obstructed. Probably estimates the centre location
            mapPosition = viewport->getCentreMapPosition();
        }
        else
        {
            mapPosition = res->first;
        }

        GameCommands::do_73(mapPosition);
    }

    // 0x0046DC9F
    // loc : gGameCommandMapX/Y/Z global
    // company : updatingCompanyId global
    // amount : ebx
    void spendMoneyEffect(const Map::Pos3& loc, const CompanyId_t company, const currency32_t amount)
    {
        if (isEditorMode())
        {
            return;
        }
        Map::Pos3 pos = loc;
        if (loc.x == Location::null)
        {
            auto* view = Ui::WindowManager::getMainViewport();
            if (view == nullptr)
            {
                return;
            }

            auto centre = view->getCentreScreenMapPosition();
            if (!centre)
            {
                return;
            }
            pos = Map::Pos3(centre->x, centre->y, Map::TileManager::getHeight(*centre).landHeight);
        }

        pos.z += 10;

        MoneyEffect::create(pos, company, -amount);
    }

    // 0x0046DE2B
    // id : updatingCompanyId global var
    // payment : ebx (subtracted from company balance)
    // type : gGameCommandExpenditureType global var
    void applyPaymentToCompany(const CompanyId_t id, const currency32_t payment, const ExpenditureType type)
    {
        auto* company = get(id);
        if (company == nullptr || OpenLoco::isEditorMode())
            return;

        WindowManager::invalidate(WindowType::company, id);

        // Invalidate the company balance if this is the player company
        if (getControllingId() == id)
        {
            Ui::Windows::PlayerInfoPanel::invalidateFrame();
        }
        auto cost = currency48_t{ payment };
        company->cash -= cost;
        company->expenditures[0][static_cast<uint8_t>(type)] -= payment;
    }

    // 0x004302EF
    void updateColours()
    {
        size_t index = 0;
        for (auto& company : rawCompanies())
        {
            _company_colours[index] = company.mainColours.primary;
            index++;
        }
        _company_colours[CompanyId::neutral] = 1;
    }

    uint32_t competingColourMask(CompanyId_t companyId)
    {
        const uint32_t similarColourMask[] = {
            0b11,
            0b11,
            0b100,
            0b11000,
            0b11000,
            0b100000,
            0b11000000,
            0b11000000,
            0b1100000000,
            0b1100000000,
            0b11110000000000,
            0b11110000000000,
            0b11110000000000,
            0b11110000000000,
            0b1100000000000000,
            0b1100000000000000,
            0b10110000000000000000,
            0b10110000000000000000,
            0b101000000000000000000,
            0b10110000000000000000,
            0b101000000000000000000,
            0b11000000000000000000000,
            0b11000000000000000000000,
            0b100000000000000000000000,
            0b1000000000000000000000000,
            0b10000000000000000000000000,
            0b1100000000000000000000000000,
            0b1100000000000000000000000000,
            0b110000000000000000000000000000,
            0b110000000000000000000000000000,
            0b1000000000000000000000000000000,
        };

        uint32_t mask = 0;
        for (auto& company : companies())
        {
            if (company.id() == companyId)
                continue;

            mask |= similarColourMask[company.mainColours.primary];
        }
        return mask;
    }

    uint32_t competingColourMask()
    {
        return competingColourMask(_updating_company_id);
    }

}

namespace OpenLoco
{
    CompanyId_t Company::id() const
    {
        auto* first = &CompanyManager::rawCompanies()[0];
        return (CompanyId_t)(this - first);
    }
}
