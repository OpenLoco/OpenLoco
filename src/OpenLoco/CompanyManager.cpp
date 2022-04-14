#include "CompanyManager.h"
#include "Config.h"
#include "Economy/Economy.h"
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
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "OpenLoco.h"
#include "Scenario.h"
#include "ScenarioManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::CompanyManager
{
    static loco_global<uint8_t, 0x00525FCB> _byte_525FCB;
    static loco_global<uint8_t, 0x00526214> _companyCompetitionDelay;
    static loco_global<uint8_t, 0x00525FB7> _companyMaxCompeting;
    static loco_global<Colour[Limits::kMaxCompanies + 1], 0x009C645C> _companyColours;
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;

    static void produceCompanies();

    static auto& rawCompanies()
    {
        return getGameState().companies;
    }

    static auto& rawPlayerCompanies()
    {
        return getGameState().playerCompanies;
    }

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
            rawPlayerCompanies()[0] = CompanyId(1);
            rawPlayerCompanies()[1] = CompanyId(0);
        }
        else if (isNetworked())
        {
            rawPlayerCompanies()[0] = CompanyId(0);
            rawPlayerCompanies()[1] = CompanyId(1);
        }
        else
        {
            rawPlayerCompanies()[0] = CompanyId(0);
            rawPlayerCompanies()[1] = CompanyId::null;
        }

        // Reset primary company colours.
        rawCompanies()[0].mainColours.primary = Colour::mutedSeaGreen;
        updateColours();
    }

    CompanyId getUpdatingCompanyId()
    {
        return _updatingCompanyId;
    }

    void setUpdatingCompanyId(CompanyId id)
    {
        _updatingCompanyId = id;
    }

    FixedVector<Company, Limits::kMaxCompanies> companies()
    {
        return FixedVector(rawCompanies());
    }

    Company* get(CompanyId id)
    {
        auto index = enumValue(id);
        if (index < Limits::kMaxCompanies)
        {
            return &rawCompanies()[index];
        }
        return nullptr;
    }

    CompanyId getControllingId()
    {
        return rawPlayerCompanies()[0];
    }

    CompanyId getSecondaryPlayerId()
    {
        return rawPlayerCompanies()[1];
    }

    void setControllingId(CompanyId id)
    {
        rawPlayerCompanies()[0] = id;
    }

    void setSecondaryPlayerId(CompanyId id)
    {
        rawPlayerCompanies()[1] = id;
    }

    Company* getPlayerCompany()
    {
        return get(rawPlayerCompanies()[0]);
    }

    Colour getCompanyColour(CompanyId id)
    {
        return _companyColours[enumValue(id)];
    }

    Colour getPlayerCompanyColour()
    {
        return _companyColours[enumValue(rawPlayerCompanies()[0])];
    }

    bool isPlayerCompany(CompanyId id)
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
            CompanyId id = CompanyId(ScenarioManager::getScenarioTicks() & 0x0F);
            auto company = get(id);
            if (company != nullptr && !isPlayerCompany(id) && !company->empty())
            {
                setUpdatingCompanyId(id);
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

    // 0x00437FB8
    void updateDaily()
    {
        for (auto& company : companies())
        {
            company.updateDaily();
        }
    }

    // 0x0043037B
    void updateMonthly1()
    {
        call(0x0043037B);
    }

    // 0x0042F213
    void updateMonthlyHeadquarters()
    {
        for (auto& company : companies())
        {
            company.updateMonthlyHeadquarters();
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

    // 0x004312C7
    void updateYearly()
    {
        call(0x004312C7);
    }

    // 0x0042F9CB
    static LoadedObjectId selectNewCompetitor(int32_t ebp)
    {
        registers regs;
        regs.ebp = ebp;
        call(0x0042F9CB, regs);
        return static_cast<LoadedObjectId>(regs.al);
    }

    static CompanyId createCompany(LoadedObjectId competitorId, bool isPlayer)
    {
        registers regs;
        regs.dl = competitorId;
        regs.dh = isPlayer ? 1 : 0;
        call(0x0042FE06, regs);
        return static_cast<CompanyId>(regs.al);
    }

    static void sub_4A6DA9()
    {
        call(0x004A6DA9);
    }

    // 0x0042F863
    void createPlayerCompany()
    {
        // Original network logic removed
        auto& gameState = getGameState();
        gameState.flags |= Scenario::Flags::preferredOwnerName;
        auto competitorId = selectNewCompetitor(-1);
        gameState.playerCompanies[0] = createCompany(competitorId, true);
        gameState.playerCompanies[1] = CompanyId::null;
        sub_4A6DA9();
    }

    // 0x0042F9AC
    static void createAiCompany()
    {
        auto competitorId = selectNewCompetitor(-1);
        if (competitorId != NullObjectId)
        {
            createCompany(competitorId, false);
        }
    }

    // 0x0042F23C
    currency32_t calculateDeliveredCargoPayment(uint8_t cargoItem, int32_t numUnits, int32_t distance, uint16_t numDays)
    {
        const auto* cargoObj = ObjectManager::get<CargoObject>(cargoItem);
        // Shift payment factor by 16 for integer maths
        auto paymentFactorPercent = cargoObj->paymentFactor << 16;

        // Integer maths for updating payment factor percentage
        // the percentage is 0 - 65535
        // Ultimate identical to floating point paymentfactor * (1-(percentage/65535))
        const auto updatePaymentFactorPercent = [&](int32_t percentage) {
            paymentFactorPercent = std::max(0, paymentFactorPercent - cargoObj->paymentFactor * percentage);
        };

        // Payment is split into 3 categories
        // Premium : Full Payment
        // NonPremium : Reduced Payment rate based on num days passed premium
        // Penalty : Further reduced payment rate based on num days passed max non premium (capped at 255)
        auto nonPremiumDays = numDays - cargoObj->premiumDays;
        if (nonPremiumDays > 0)
        {
            updatePaymentFactorPercent(cargoObj->nonPremiumRate * std::min<int32_t>(nonPremiumDays, cargoObj->maxNonPremiumDays));
            auto penaltyDays = std::min(255, nonPremiumDays - cargoObj->maxNonPremiumDays);
            if (penaltyDays > 0)
            {
                updatePaymentFactorPercent(cargoObj->penaltyRate * penaltyDays);
            }
        }
        paymentFactorPercent >>= 16;
        // Promote to 64bit for second part of payment calc.
        const auto unitDistancePayment = static_cast<int64_t>(Economy::getInflationAdjustedCost(paymentFactorPercent, cargoObj->paymentIndex, 8));
        const auto payment = (unitDistancePayment * numUnits * distance) / 4096;
        return payment;
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
        if (_companyCompetitionDelay == 0 && _companyMaxCompeting != 0)
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
                if (prng.randNext(_companyMaxCompeting) + 1 > companies_active)
                {
                    createAiCompany();
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
    string_id getOwnerStatus(CompanyId id, FormatArguments& args)
    {
        auto* company = get(id);
        if (company == nullptr)
        {
            return StringIds::company_status_empty;
        }

        if (company->challengeFlags & CompanyFlags::bankrupt)
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

        auto company = CompanyManager::get(_updatingCompanyId);
        if (company == nullptr)
        {
            return;
        }

        company->updateCounter += 1;
        if ((company->updateCounter % 128) != 0)
            return;

        for (size_t i = 0; i < WindowManager::count(); i++)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            auto* vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(w->number));
            if (vehicle == nullptr)
                continue;

            if (vehicle->position.x == Location::null)
                continue;

            if (vehicle->owner != _updatingCompanyId)
                continue;

            GameCommands::UpdateOwnerStatusArgs args{};
            args.ownerStatus = OwnerStatus(vehicle->id);

            GameCommands::doCommand(args, GameCommands::Flags::apply);
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

        GameCommands::UpdateOwnerStatusArgs args{};
        args.ownerStatus = OwnerStatus(mapPosition);

        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    // 0x0046DC9F
    // loc : gGameCommandMapX/Y/Z global
    // company : updatingCompanyId global
    // amount : ebx
    void spendMoneyEffect(const Map::Pos3& loc, const CompanyId company, const currency32_t amount)
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
    void applyPaymentToCompany(const CompanyId id, const currency32_t payment, const ExpenditureType type)
    {
        auto* company = get(id);
        if (company == nullptr || OpenLoco::isEditorMode())
            return;

        WindowManager::invalidate(WindowType::company, enumValue(id));

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
            _companyColours[index] = company.mainColours.primary;
            index++;
        }
        _companyColours[enumValue(CompanyId::neutral)] = Colour::grey;
    }

    // 0x004C95A6
    void setPreferredName()
    {
        if (!(Config::get().flags & Config::Flags::usePreferredOwnerName))
            return;

        // First, set the owner name.
        GameCommands::setErrorTitle(StringIds::cannot_change_owner_name);
        {
            const uint32_t* buffer = reinterpret_cast<uint32_t*>(Config::get().preferredName);
            GameCommands::do_31(_updatingCompanyId, 1, buffer[0], buffer[1], buffer[2]);
            GameCommands::do_31(CompanyId(0), 2, buffer[3], buffer[4], buffer[5]);
            if (GameCommands::do_31(CompanyId(0), 0, buffer[6], buffer[7], buffer[8]))
                Ui::Windows::TextInput::cancel();
        }

        // Only continue if we've not set a custom company name yet.
        auto* company = get(_updatingCompanyId);
        if (company == nullptr || company->name != StringIds::new_company)
            return;

        // Temporarily store the preferred name in buffer string 2039.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(buffer_2039, Config::get().preferredName, 256);

        // Prepare '{NAME} Transport' in a buffer.
        {
            char companyName[256] = { 0 };
            auto args = FormatArguments::common(StringIds::buffer_2039);
            StringManager::formatString(companyName, StringIds::company_owner_name_transport, &args);

            // Now, set the company name.
            const uint32_t* buffer = reinterpret_cast<uint32_t*>(companyName);
            GameCommands::setErrorTitle(StringIds::cannot_rename_this_company);
            GameCommands::do_30(_updatingCompanyId, 1, buffer[0], buffer[1], buffer[2]);
            GameCommands::do_30(CompanyId(0), 2, buffer[3], buffer[4], buffer[5]);
            GameCommands::do_30(CompanyId(0), 0, buffer[6], buffer[7], buffer[8]);
        }
    }

    uint32_t competingColourMask(CompanyId companyId)
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

            mask |= similarColourMask[enumValue(company.mainColours.primary)];
        }
        return mask;
    }

    uint32_t competingColourMask()
    {
        return competingColourMask(_updatingCompanyId);
    }

    // 0x00434F2D
    uint8_t getHeadquarterBuildingType()
    {
        for (size_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::building); ++i)
        {
            auto* buildingObj = ObjectManager::get<BuildingObject>(i);
            if (buildingObj == nullptr)
            {
                continue;
            }

            if (buildingObj->flags & BuildingObjectFlags::is_headquarters)
            {
                return static_cast<uint8_t>(i);
            }
        }
        return 0;
    }

}

namespace OpenLoco
{
    CompanyId Company::id() const
    {
        auto* first = &CompanyManager::rawCompanies()[0];
        return CompanyId(this - first);
    }
}
