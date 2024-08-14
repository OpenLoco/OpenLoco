#include "CompanyManager.h"
#include "CompanyAi.h"
#include "CompanyRecords.h"
#include "Config.h"
#include "Economy/Economy.h"
#include "Effects/Effect.h"
#include "Effects/MoneyEffect.h"
#include "Entities/EntityManager.h"
#include "GameCommands/Company/RenameCompanyName.h"
#include "GameCommands/Company/RenameCompanyOwner.h"
#include "GameCommands/Company/UpdateOwnerStatus.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Graphics/Colour.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Objects/AirportObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "Scenario.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "TownManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Bound.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::CompanyManager
{
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

        getGameState().produceAICompanyTimeout = 0;

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

    // 0x00525FB7
    uint8_t getMaxCompetingCompanies()
    {
        return getGameState().maxCompetingCompanies;
    }
    void setMaxCompetingCompanies(uint8_t competingCompanies)
    {
        getGameState().maxCompetingCompanies = competingCompanies;
    }

    // 0x00526214
    uint8_t getCompetitorStartDelay()
    {
        return getGameState().competitorStartDelay;
    }
    void setCompetitorStartDelay(uint8_t competetorStartDelay)
    {
        getGameState().competitorStartDelay = competetorStartDelay;
    }

    // 0x0052621A
    uint16_t getMaxLoanSize()
    {
        return getGameState().maxLoanSize;
    }
    void setMaxLoanSize(uint16_t loanSize)
    {
        getGameState().maxLoanSize = loanSize;
    }

    // 0x00526218
    uint16_t getStartingLoanSize()
    {
        return getGameState().startingLoanSize;
    }
    void setStartingLoanSize(uint16_t loanSize)
    {
        getGameState().startingLoanSize = loanSize;
    }

    const Records& getRecords()
    {
        return getGameState().companyRecords;
    }
    void setRecords(const Records& records)
    {
        getGameState().companyRecords = records;
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
        if (!isEditorMode() && !Config::get().companyAIDisabled)
        {
            CompanyId id = CompanyId(ScenarioManager::getScenarioTicks() & 0x0F);
            auto company = get(id);
            if (company != nullptr && !isPlayerCompany(id) && !company->empty())
            {
                // Only the host should update AI, AI will run game commands
                // which will be sent to all the clients
                if (!isNetworked() || isNetworkHost())
                {
                    setUpdatingCompanyId(id);
                    aiThink(id);
                }
            }

            getGameState().produceAICompanyTimeout++;
            if (getGameState().produceAICompanyTimeout >= 192)
            {
                getGameState().produceAICompanyTimeout = 0;
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
        setCompetitorStartDelay(Math::Bound::sub(getCompetitorStartDelay(), 1U));

        for (auto& company : companies())
        {
            company.updateMonthly1();
        }
        Ui::WindowManager::invalidate(Ui::WindowType::company);
        Ui::WindowManager::invalidate(Ui::WindowType::companyList);
        Ui::WindowManager::invalidate(Ui::WindowType::playerInfoToolbar);

        uint8_t numActiveCompanies = std::distance(std::begin(companies()), std::end(companies()));

        auto minPerformance = std::min_element(std::begin(companies()), std::end(companies()), [](const Company& lhs, const Company& rhs) {
            return lhs.performanceIndex < rhs.performanceIndex;
        });
        if (minPerformance != std::end(companies()) && numActiveCompanies != 1)
        {
            companyEmotionEvent((*minPerformance).id(), Emotion::dejected);
        }
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
        for (auto& company : companies())
        {
            updateYearly(company);
        }
        WindowManager::invalidate(Ui::WindowType::company);
    }

    // 0x0042F9CB
    static LoadedObjectId selectNewCompetitor(ObjectHeader* header = nullptr)
    {
        registers regs;
        if (header != nullptr)
            regs.ebp = X86Pointer(header);
        else
            regs.ebp = -1;

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
        gameState.flags |= GameStateFlags::preferredOwnerName;

        // Any preference with respect to owner face?
        ObjectHeader* preferredOwner = nullptr;
        if (Config::get().usePreferredOwnerFace)
        {
            preferredOwner = &Config::get().preferredOwnerFace;
        }

        auto competitorId = selectNewCompetitor(preferredOwner);
        // This might happen if a preferredOwner object header does not exist anymore.
        if (competitorId == kNullObjectId)
        {
            competitorId = selectNewCompetitor(nullptr);
        }
        gameState.playerCompanies[0] = createCompany(competitorId, true);
        gameState.playerCompanies[1] = CompanyId::null;
        sub_4A6DA9();
    }

    // 0x0042F9AC
    static void createAiCompany()
    {
        auto competitorId = selectNewCompetitor();
        if (competitorId != kNullObjectId)
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
        // Ultimate identical to floating point paymentFactor * (1-(percentage/65535))
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
        if (getCompetitorStartDelay() == 0 && getMaxCompetingCompanies() != 0)
        {
            int32_t activeCompanies = 0;
            for (const auto& company : companies())
            {
                if (!isPlayerCompany(company.id()))
                    activeCompanies++;
            }

            auto& prng = gPrng1();
            if (prng.randNext(16) == 0)
            {
                if (prng.randNext(getMaxCompetingCompanies()) + 1 > activeCompanies)
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
    StringId getOwnerStatus(CompanyId id, FormatArguments& args)
    {
        auto* company = get(id);
        if (company == nullptr)
        {
            return StringIds::company_status_empty;
        }

        if ((company->challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
            return StringIds::company_status_bankrupt;

        const StringId observationStatusStrings[] = {
            StringIds::company_status_empty,
            StringIds::company_status_building_track_road,
            StringIds::company_status_building_airport,
            StringIds::company_status_building_dock,
            StringIds::company_status_checking_services,
            StringIds::company_status_surveying_landscape,
        };

        StringId statusString = observationStatusStrings[company->observationStatus];
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

        World::Pos2 mapPosition{};
        if (!res || res->second != viewport)
        {
            // Happens if centre of viewport is obstructed. Probably estimates the centre location
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
    void spendMoneyEffect(const World::Pos3& loc, const CompanyId company, const currency32_t amount)
    {
        if (isEditorMode())
        {
            return;
        }
        World::Pos3 pos = loc;
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
            pos = World::Pos3(centre->x, centre->y, World::TileManager::getHeight(*centre).landHeight);
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

    constexpr currency32_t kAiLoanStep = 1000;

    // 0x0046DD06
    // id : updatingCompanyId global var
    // payment : ebp
    // return : ebp == 0x80000000 for false
    bool ensureCompanyFunding(const CompanyId id, const currency32_t payment)
    {
        if (payment <= 0)
        {
            return true;
        }
        if (isEditorMode())
        {
            return true;
        }
        if (id == CompanyId::neutral)
        {
            return true;
        }

        auto* company = get(id);
        if (isPlayerCompany(id))
        {
            if ((company->challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
            {
                GameCommands::setErrorText(StringIds::company_is_bankrupt);
                return false;
            }
            if (company->cash < payment)
            {
                FormatArguments::common(payment);
                GameCommands::setErrorText(StringIds::not_enough_cash_requires_currency32);
                return false;
            }
            return true;
        }
        else
        {
            if (company->cash < payment)
            {

                const auto requiredAdditionalFunds = -(company->cash - payment).asInt64();
                if (requiredAdditionalFunds > 0)
                {
                    // Round up to nearest kAiLoanStep
                    const auto requiredAdditionalLoan = ((requiredAdditionalFunds + kAiLoanStep - 1) / kAiLoanStep) * kAiLoanStep;

                    const auto maxLoan = Economy::getInflationAdjustedCost(CompanyManager::getMaxLoanSize(), 0, 8);
                    if (requiredAdditionalLoan + company->currentLoan > maxLoan)
                    {
                        FormatArguments::common(payment);
                        GameCommands::setErrorText(StringIds::not_enough_cash_requires_currency32);
                        return false;
                    }

                    company->currentLoan += requiredAdditionalLoan;
                    company->cash += requiredAdditionalLoan;
                    Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(id));
                }
            }
            return true;
        }
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
        if (!Config::get().usePreferredOwnerName)
            return;

        // First, set the owner name.
        GameCommands::setErrorTitle(StringIds::cannot_change_owner_name);
        {
            GameCommands::ChangeCompanyOwnerNameArgs args{};

            args.companyId = CompanyId(_updatingCompanyId);
            args.bufferIndex = 1;
            std::memcpy(args.newName, Config::get().preferredOwnerName.c_str(), 36);

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.bufferIndex = 2;

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.bufferIndex = 0;

            if (GameCommands::doCommand(args, GameCommands::Flags::apply))
            {
                Ui::Windows::TextInput::cancel();
            }
        }

        // Only continue if we've not set a custom company name yet.
        auto* company = get(_updatingCompanyId);
        if (company == nullptr || company->name != StringIds::new_company)
            return;

        // Temporarily store the preferred name in buffer string 2039.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(buffer_2039, Config::get().old.preferredName, 256);

        // Prepare '{NAME} Transport' in a buffer.
        {
            char companyName[256] = { 0 };
            FormatArguments args{};
            args.push(StringIds::buffer_2039);
            StringManager::formatString(companyName, StringIds::company_owner_name_transport, args);

            // Now, set the company name.

            GameCommands::setErrorTitle(StringIds::cannot_rename_this_company);

            GameCommands::ChangeCompanyNameArgs changeCompanyNameArgs{};

            changeCompanyNameArgs.companyId = CompanyId(_updatingCompanyId);
            changeCompanyNameArgs.bufferIndex = 1;
            std::memcpy(changeCompanyNameArgs.buffer, companyName, 36);

            GameCommands::doCommand(changeCompanyNameArgs, GameCommands::Flags::apply);

            changeCompanyNameArgs.bufferIndex = 2;

            GameCommands::doCommand(changeCompanyNameArgs, GameCommands::Flags::apply);

            changeCompanyNameArgs.bufferIndex = 0;

            GameCommands::doCommand(changeCompanyNameArgs, GameCommands::Flags::apply);
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

            if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
            {
                return static_cast<uint8_t>(i);
            }
        }
        return 0;
    }

    // 0x004353F4
    std::vector<uint32_t> findAllOtherInUseCompetitors(const CompanyId id)
    {
        std::vector<uint8_t> takenCompetitorIds;
        for (const auto& c : companies())
        {
            if (c.id() != id)
            {
                takenCompetitorIds.push_back(c.competitorId);
            }
        }

        std::vector<uint32_t> inUseCompetitors;
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
        {
            auto competitorId = ObjectManager::findObjectHandle(*object.second._header);
            if (competitorId)
            {
                auto res = std::find(takenCompetitorIds.begin(), takenCompetitorIds.end(), competitorId->id);
                if (res != takenCompetitorIds.end())
                {
                    inUseCompetitors.push_back(object.first);
                }
            }
        }
        return inUseCompetitors;
    }

    // 0x00435AEF
    void aiDestroy(const CompanyId id)
    {
        auto* company = get(id);
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00435AEF, regs);
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
