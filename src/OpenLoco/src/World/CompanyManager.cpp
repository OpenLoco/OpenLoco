#include "CompanyManager.h"
#include "CompanyAi.h"
#include "CompanyRecords.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Effects/Effect.h"
#include "Effects/MoneyEffect.h"
#include "Entities/EntityManager.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
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
#include "MessageManager.h"
#include "Objects/AirportObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/CompetitorObject.h"
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
#include "Ui/Dropdown.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Bound.hpp>
#include <sfl/static_vector.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::CompanyManager
{
    static loco_global<Colour[Limits::kMaxCompanies + 1], 0x009C645C> _companyColours;

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
        {
            company.name = StringIds::empty;
        }

        getGameState().produceAICompanyTimeout = 0;

        // Reset player companies depending on network mode.
        if (SceneManager::isNetworkHost())
        {
            rawPlayerCompanies()[0] = CompanyId(1);
            rawPlayerCompanies()[1] = CompanyId(0);
        }
        else if (SceneManager::isNetworked())
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

    void removeCompaniesRecords(CompanyId id)
    {
        auto records = getRecords();
        for (auto i = 0U; i < 3; ++i)
        {
            if (records.speed[i] == kSpeedZero)
            {
                continue;
            }
            if (records.company[i] == id)
            {
                records.company[i] = CompanyId::null;
            }
        }
        setRecords(records);
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
        if (!SceneManager::isEditorMode() && !Config::get().companyAIDisabled)
        {
            CompanyId id = CompanyId(ScenarioManager::getScenarioTicks() & 0x0F);
            auto company = get(id);
            if (company != nullptr && !isPlayerCompany(id) && !company->empty())
            {
                // Only the host should update AI, AI will run game commands
                // which will be sent to all the clients
                if (!SceneManager::isNetworked() || SceneManager::isNetworkHost())
                {
                    GameCommands::setUpdatingCompanyId(id);
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

    constexpr std::array<uint8_t, 10> kAiToMetric = { 1, 1, 1, 1, 2, 2, 2, 3, 3, 3 };

    // 0x0042FD62
    static LoadedObjectId selectNewCompetitorFromHeader(const ObjectHeader& header)
    {
        auto loadedObj = ObjectManager::findObjectHandle(header);
        if (loadedObj.has_value())
        {
            return loadedObj.value().id;
        }

        bool freeSlot = false;
        for (LoadedObjectId id = 0U; id < ObjectManager::getMaxObjects(ObjectType::competitor); ++id)
        {
            auto* competitorObj = ObjectManager::get<CompetitorObject>(id);
            if (competitorObj == nullptr)
            {
                freeSlot = true;
                break;
            }
        }
        if (!freeSlot)
        {
            LoadedObjectId id = 0U;
            for (; id < ObjectManager::getMaxObjects(ObjectType::competitor); ++id)
            {
                const bool isUnused = std::none_of(companies().begin(), companies().end(), [id](Company& company) { return company.competitorId == id; });
                if (isUnused)
                {
                    break;
                }
            }
            if (id == ObjectManager::getMaxObjects(ObjectType::competitor))
            {
                return kNullObjectId;
            }

            ObjectManager::unload(ObjectManager::getHeader(LoadedObjectHandle{
                ObjectType::competitor, id }));
            ObjectManager::reloadAll();
            Ui::Dropdown::forceCloseCompanySelect();
        }
        ObjectManager::load(header);
        ObjectManager::reloadAll();
        Ui::Dropdown::forceCloseCompanySelect();
        loadedObj = ObjectManager::findObjectHandle(header);
        if (loadedObj.has_value())
        {
            return loadedObj.value().id;
        }
        return kNullObjectId;
    }

    // 0x0042F9CB
    static LoadedObjectId selectNewCompetitor()
    {
        sfl::static_vector<LoadedObjectId, ObjectManager::getMaxObjects(ObjectType::competitor)> loadedUnusedCompetitors;
        for (LoadedObjectId id = 0U; id < ObjectManager::getMaxObjects(ObjectType::competitor); ++id)
        {
            auto* competitorObj = ObjectManager::get<CompetitorObject>(id);
            if (competitorObj == nullptr)
            {
                continue;
            }
            const bool isFree = std::none_of(companies().begin(), companies().end(), [id](Company& company) { return company.competitorId == id; });
            if (isFree)
            {
                loadedUnusedCompetitors.push_back(id);
            }
        }

        if (!loadedUnusedCompetitors.empty())
        {
            const auto r = ((gPrng1().randNext() & 0xFFFFU) * loadedUnusedCompetitors.size()) / 65536;
            return loadedUnusedCompetitors[r];
        }

        std::optional<std::vector<ObjectHeader>> bestInstalled = std::nullopt;
        uint8_t bestInstalledValue = 0xFFU;
        auto installedCompetitors = ObjectManager::getAvailableObjects(ObjectType::competitor);
        for (auto& installed : installedCompetitors)
        {
            bool isInUse = false;
            for (LoadedObjectId id = 0U; id < ObjectManager::getMaxObjects(ObjectType::competitor); ++id)
            {
                if (ObjectManager::get<CompetitorObject>(id) == nullptr)
                {
                    continue;
                }
                auto& loadedHeader = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::competitor, id });
                if (loadedHeader == installed.object._header)
                {
                    isInUse = true;
                    break;
                }
            }
            if (isInUse)
            {
                continue;
            }

            uint8_t metric = 0;
            if (getGameState().preferredAIIntelligence != 0)
            {
                metric += std::abs(kAiToMetric[installed.object._displayData.intelligence] - getGameState().preferredAIIntelligence);
            }
            if (getGameState().preferredAIAggressiveness != 0)
            {
                metric += std::abs(kAiToMetric[installed.object._displayData.aggressiveness] - getGameState().preferredAIAggressiveness);
            }
            if (getGameState().preferredAICompetitiveness != 0)
            {
                metric += std::abs(kAiToMetric[installed.object._displayData.competitiveness] - getGameState().preferredAICompetitiveness);
            }
            if (metric < bestInstalledValue)
            {
                bestInstalled = std::vector<ObjectHeader>{ installed.object._header };
                bestInstalledValue = metric;
            }
            else if (metric == bestInstalledValue)
            {
                assert(bestInstalled.has_value());
                bestInstalled->push_back(installed.object._header);
            }
        }

        if (!bestInstalled.has_value())
        {
            return kNullObjectId;
        }
        const auto r = ((gPrng1().randNext() & 0xFFFFU) * bestInstalled->size()) / 65536;
        const auto& chosenHeader = (*bestInstalled)[r];
        return selectNewCompetitorFromHeader(chosenHeader);
    }

    // 0x004F93F8
    static constexpr std::array<Colour, 13> kAiPrimaryColours = {
        Colour::black,
        Colour::grey,
        Colour::white,
        Colour::mutedDarkPurple,
        Colour::blue,
        Colour::green,
        Colour::amber,
        Colour::darkOrange,
        Colour::mutedYellow,
        Colour::mutedDarkRed,
        Colour::red,
        Colour::max, // Pick randomcolour
        Colour::max, // Pick randomcolour
    };

    // 0x004F9405
    static constexpr std::array<Colour, 31> kPrimaryToSecondary = {
        Colour::mutedDarkPurple,
        Colour::mutedDarkRed,
        Colour::mutedTeal,
        Colour::white,
        Colour::mutedPurple,
        Colour::white,
        Colour::white,
        Colour::blue,
        Colour::white,
        Colour::darkRed,
        Colour::mutedGrassGreen,
        Colour::white,
        Colour::yellow,
        Colour::mutedGrassGreen,
        Colour::darkGreen,
        Colour::yellow,
        Colour::red,
        Colour::mutedOrange,
        Colour::mutedYellow,
        Colour::amber,
        Colour::white,
        Colour::mutedDarkRed,
        Colour::white,
        Colour::yellow,
        Colour::white,
        Colour::white,
        Colour::yellow,
        Colour::white,
        Colour::darkPink,
        Colour::white,
        Colour::mutedDarkRed,
    };

    // 0x004F93C4
    static constexpr std::array<AiPlaystyleFlags, 13> kCompanyAiPlaystyleFlags = {
        AiPlaystyleFlags::none,
        AiPlaystyleFlags::none,
        AiPlaystyleFlags::none,
        AiPlaystyleFlags::unk1 | AiPlaystyleFlags::unk2 | AiPlaystyleFlags::unk4 | AiPlaystyleFlags::unk5,
        AiPlaystyleFlags::unk0 | AiPlaystyleFlags::unk2 | AiPlaystyleFlags::unk4 | AiPlaystyleFlags::unk5,
        AiPlaystyleFlags::unk0 | AiPlaystyleFlags::unk1 | AiPlaystyleFlags::unk2 | AiPlaystyleFlags::unk3 | AiPlaystyleFlags::unk5,
        AiPlaystyleFlags::unk1 | AiPlaystyleFlags::unk2 | AiPlaystyleFlags::unk4 | AiPlaystyleFlags::unk5,
        AiPlaystyleFlags::unk6,
        AiPlaystyleFlags::unk1 | AiPlaystyleFlags::unk2 | AiPlaystyleFlags::unk4 | AiPlaystyleFlags::unk5,
        AiPlaystyleFlags::unk6,
        AiPlaystyleFlags::unk6,
        AiPlaystyleFlags::unk6,
        AiPlaystyleFlags::unk0 | AiPlaystyleFlags::unk1 | AiPlaystyleFlags::unk3 | AiPlaystyleFlags::unk4 | AiPlaystyleFlags::unk5,
    };

    static constexpr std::array<StringId, 13> kCompanyAiNamePrefixes = {
        StringIds::company_ai_name_ebony,
        StringIds::company_ai_name_silver,
        StringIds::company_ai_name_ivory,
        StringIds::company_ai_name_indigo,
        StringIds::company_ai_name_sapphire,
        StringIds::company_ai_name_emerald,
        StringIds::company_ai_name_golden,
        StringIds::company_ai_name_amber,
        StringIds::company_ai_name_bronze,
        StringIds::company_ai_name_burgundy,
        StringIds::company_ai_name_scarlet,
        StringIds::company_ai_name_string,
        StringIds::company_ai_name_pop_string,
    };

    static constexpr std::array<StringId, 13> kCompanyAiPlaystyleString = {
        StringIds::company_ai_name_string_transport,
        StringIds::company_ai_name_string_express,
        StringIds::company_ai_name_string_lines,
        StringIds::company_ai_name_string_tracks,
        StringIds::company_ai_name_string_coaches,
        StringIds::company_ai_name_string_air,
        StringIds::company_ai_name_string_rail,
        StringIds::company_ai_name_string_carts,
        StringIds::company_ai_name_string_trains,
        StringIds::company_ai_name_string_haulage,
        StringIds::company_ai_name_string_shipping,
        StringIds::company_ai_name_string_freight,
        StringIds::company_ai_name_string_trucks,
    };

    // 0x0042FE06
    static CompanyId createCompany(LoadedObjectId competitorId, bool isPlayer)
    {
        auto chosenCompanyId = []() {
            for (auto& company : rawCompanies())
            {
                if (company.empty())
                {
                    return company.id();
                }
            }
            return CompanyId::null;
        }();
        if (chosenCompanyId == CompanyId::null)
        {
            return CompanyId::null;
        }

        auto* company = get(chosenCompanyId);
        company->competitorId = competitorId;
        auto* competitorObj = ObjectManager::get<CompetitorObject>(competitorId);
        company->challengeFlags = CompanyFlags::none;
        company->var_49C = 0;
        company->var_4A0 = 0;
        company->ownerEmotion = Emotion::neutral;
        company->name = StringIds::new_company;
        company->ownerName = StringIds::new_owner;
        company->startedDate = getCurrentDay();
        if (isPlayer)
        {
            Colour primaryColour = Colour::max;
            while (true)
            {
                const auto randVal = gPrng1().randNext();

                primaryColour = static_cast<Colour>(((randVal & 0xFFU) * enumValue(Colour::max)) / 256);

                const auto colourMask = competingColourMask(chosenCompanyId);
                if (colourMask & (1U << enumValue(primaryColour)))
                {
                    continue;
                }
                break;
            }

            const auto secondaryColour = kPrimaryToSecondary[enumValue(primaryColour)];
            const auto colourScheme = ColourScheme{ primaryColour, secondaryColour };
            company->mainColours = colourScheme;
            std::fill(std::begin(company->vehicleColours), std::end(company->vehicleColours), colourScheme);

            company->customVehicleColoursSet = 0;
        }
        else
        {
            company->ownerName = competitorObj->name;
            uint32_t randVal = 0;
            uint8_t companyNamePrefix = 0; // Usually a colour but can be a town
            uint8_t companyPlaystyle = 0;
            Colour primaryColour = Colour::max;
            bool colourOk = false;
            for (auto i = 0U; i < 250; ++i)
            {
                randVal = gPrng1().randNext();
                sfl::static_vector<uint8_t, 32> availableNamePrefixes;
                for (auto j = 0U; j < 32; ++j)
                {
                    if (competitorObj->var_04 & (1U << j))
                    {
                        availableNamePrefixes.push_back(j);
                    }
                }
                companyNamePrefix = availableNamePrefixes[availableNamePrefixes.size() * (randVal & 0xFFU) / 256];
                randVal = std::rotr(randVal, 8);

                sfl::static_vector<uint8_t, 32> availablePlaystyles;
                for (auto j = 0U; j < 32; ++j)
                {
                    if (competitorObj->var_08 & (1U << j))
                    {
                        availablePlaystyles.push_back(j);
                    }
                }
                companyPlaystyle = availablePlaystyles[availablePlaystyles.size() * (randVal & 0xFFU) / 256];
                randVal = std::rotr(randVal, 8);

                primaryColour = kAiPrimaryColours[companyNamePrefix];
                if (primaryColour == Colour::max)
                {
                    primaryColour = static_cast<Colour>((randVal & 0xFFU) * 31 / 256);
                    randVal = std::rotr(randVal, 8);
                }

                const auto colourMask = competingColourMask(chosenCompanyId);
                if (colourMask & (1U << enumValue(primaryColour)))
                {
                    continue;
                }
                colourOk = true;
                break;
            }
            if (!colourOk)
            {
                company->name = StringIds::empty;
                return CompanyId::null;
            }

            const auto secondaryColour = kPrimaryToSecondary[enumValue(primaryColour)];
            const auto colourScheme = ColourScheme{ primaryColour, secondaryColour };
            company->mainColours = colourScheme;
            std::fill(std::begin(company->vehicleColours), std::end(company->vehicleColours), colourScheme);

            company->customVehicleColoursSet = 0;
            company->aiPlaystyleFlags = kCompanyAiPlaystyleFlags[companyPlaystyle];
            company->aiPlaystyleTownId = 0xFFU;

            if (companyNamePrefix == 12)
            {
                const auto numTowns = TownManager::towns().size();
                if (numTowns == 0)
                {
                    company->name = StringIds::empty;
                    return CompanyId::null;
                }
                auto randTown = (((randVal & 0xFFU) * numTowns) / 256) + 1;
                randVal = std::rotr(randVal, 8);
                const auto randTownId = [&randTown]() {
                    for (auto& town : TownManager::towns())
                    {
                        randTown--;
                        if (randTown == 0)
                        {
                            return town.id();
                        }
                    }
                    return TownId::null;
                }();
                for (auto& otherCompany : companies())
                {
                    if (otherCompany.id() == chosenCompanyId)
                    {
                        continue;
                    }
                    if ((otherCompany.aiPlaystyleFlags & AiPlaystyleFlags::townIdSet) != AiPlaystyleFlags::none)
                    {
                        continue;
                    }
                    if (static_cast<TownId>(otherCompany.aiPlaystyleTownId) == randTownId)
                    {
                        company->name = StringIds::empty;
                        return CompanyId::null;
                    }
                }
                company->aiPlaystyleTownId = enumValue(randTownId);
                company->aiPlaystyleFlags |= AiPlaystyleFlags::townIdSet;
            }

            const auto stringId = kCompanyAiPlaystyleString[companyPlaystyle];
            auto args = FormatArguments::common(kCompanyAiNamePrefixes[companyNamePrefix], competitorObj->lastName);
            if (company->aiPlaystyleTownId != 0xFFU)
            {
                args.push(TownManager::get(static_cast<TownId>(company->aiPlaystyleTownId))->name);
            }

            char buffer[256]{};
            StringManager::formatString(buffer, 256U, stringId, args);
            for (auto& otherCompany : companies())
            {
                if (otherCompany.id() == chosenCompanyId)
                {
                    continue;
                }

                char buffer2[256]{};
                StringManager::formatString(buffer2, 256U, otherCompany.name);
                if (std::strncmp(buffer, buffer2, 256) == 0)
                {
                    company->name = StringIds::empty;
                    return CompanyId::null;
                }
            }
            company->name = StringManager::userStringAllocate(buffer, false);
            if (company->name == StringIds::empty)
            {
                return CompanyId::null;
            }
        }
        company->numExpenditureYears = 1;
        for (auto i = 0U; i < ExpenditureType::Count; ++i)
        {
            company->expenditures[0][i] = 0;
        }
        company->var_4A4 = AiThinkState::unk0;
        company->var_4A6 = 0;
        company->var_85F6 = 0;
        for (auto& thought : company->aiThoughts)
        {
            thought.type = AiThoughtType::null;
        }
        company->headquartersX = -1;
        company->var_25BE = 0xFFU;
        company->unlockedVehicles.reset();
        company->availableVehicles = 0;
        company->currentLoan = getInflationAdjustedStartingLoan();
        company->cash = company->currentLoan;
        VehicleManager::determineAvailableVehicles(*company);
        company->cargoUnitsTotalDelivered = 0;
        company->cargoUnitsTotalDistance = 0;
        company->historySize = 1;
        company->performanceIndex = 0;
        company->performanceIndexHistory[0] = 0;
        company->cargoUnitsDeliveredHistory[0] = 0;
        std::fill(std::begin(company->transportTypeCount), std::end(company->transportTypeCount), 0);
        std::fill(std::begin(company->activeEmotions), std::end(company->activeEmotions), 0);
        const auto value = calculateCompanyValue(*company);
        company->companyValueHistory[0] = value.companyValue;
        company->vehicleProfit = value.vehicleProfit;
        updateColours();
        company->observationStatus = ObservationStatus::empty;
        company->observationEntity = EntityId::null;
        company->observationX = -1;
        company->observationY = -1;
        company->observationObject = 0xFFFFU;
        company->observationTimeout = 0;
        company->ownerStatus = OwnerStatus();
        company->updateCounter = 0;
        company->currentRating = CorporateRating::platelayer;
        company->challengeProgress = 0;
        company->numMonthsInTheRed = 0;
        company->jailStatus = 0;
        std::fill(std::begin(company->cargoDelivered), std::end(company->cargoDelivered), 0);
        for (auto& town : TownManager::towns())
        {
            town.companyRatings[enumValue(chosenCompanyId)] = 500;
            town.companiesWithRating &= ~(1 << enumValue(chosenCompanyId));
        }
        return chosenCompanyId;
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
        auto competitorId = Config::get().usePreferredOwnerFace ? selectNewCompetitorFromHeader(Config::get().preferredOwnerFace)
                                                                : selectNewCompetitor();
        // This might happen if a preferredOwner object header does not exist anymore.
        if (competitorId == kNullObjectId)
        {
            competitorId = selectNewCompetitor();
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
                {
                    activeCompanies++;
                }
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
        {
            return StringIds::company_status_bankrupt;
        }

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
        {
            return StringIds::company_status_empty;
        }

        switch (company->observationStatus)
        {
            case ObservationStatus::buildingTrackRoad:
                if (company->observationObject & 0x80)
                {
                    auto* obj = ObjectManager::get<RoadObject>(company->observationObject & 0xFF7F);
                    if (obj != nullptr)
                    {
                        args.push(obj->name);
                    }
                }
                else
                {
                    auto* obj = ObjectManager::get<TrackObject>(company->observationObject);
                    if (obj != nullptr)
                    {
                        args.push(obj->name);
                    }
                }
                break;

            case ObservationStatus::buildingAirport:
            {
                auto* obj = ObjectManager::get<AirportObject>(company->observationObject);
                if (obj != nullptr)
                {
                    args.push(obj->name);
                }
                break;
            }

            case ObservationStatus::buildingDock:
            {
                auto* obj = ObjectManager::get<DockObject>(company->observationObject);
                if (obj != nullptr)
                {
                    args.push(obj->name);
                }
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
        if (SceneManager::isTitleMode() || SceneManager::isEditorMode())
        {
            return;
        }

        auto companyId = GameCommands::getUpdatingCompanyId();
        auto company = CompanyManager::get(companyId);
        if (company == nullptr)
        {
            return;
        }

        company->updateCounter += 1;
        if ((company->updateCounter % 128) != 0)
        {
            return;
        }

        for (size_t i = 0; i < WindowManager::count(); i++)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
            {
                continue;
            }

            auto* vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(w->number));
            if (vehicle == nullptr)
            {
                continue;
            }

            if (vehicle->position.x == Location::null)
            {
                continue;
            }

            if (vehicle->owner != companyId)
            {
                continue;
            }

            GameCommands::UpdateOwnerStatusArgs args{};
            args.ownerStatus = OwnerStatus(vehicle->id);

            GameCommands::doCommand(args, GameCommands::Flags::apply);
            return;
        }

        auto main = WindowManager::getMainWindow();
        if (main == nullptr)
        {
            return;
        }

        auto viewport = main->viewports[0];
        if (viewport == nullptr)
        {
            return;
        }

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
        if (SceneManager::isEditorMode())
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
        if (company == nullptr || SceneManager::isEditorMode())
        {
            return;
        }

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
        if (SceneManager::isEditorMode())
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
        {
            return;
        }

        // First, set the owner name.
        GameCommands::setErrorTitle(StringIds::cannot_change_owner_name);
        {
            GameCommands::ChangeCompanyOwnerNameArgs args{};

            args.companyId = GameCommands::getUpdatingCompanyId();
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
        auto* company = get(GameCommands::getUpdatingCompanyId());
        if (company == nullptr || company->name != StringIds::new_company)
        {
            return;
        }

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

            changeCompanyNameArgs.companyId = GameCommands::getUpdatingCompanyId();
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
            {
                continue;
            }

            mask |= similarColourMask[enumValue(company.mainColours.primary)];
        }
        return mask;
    }

    uint32_t competingColourMask()
    {
        return competingColourMask(GameCommands::getUpdatingCompanyId());
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
            auto competitorId = ObjectManager::findObjectHandle(object.object._header);
            if (competitorId)
            {
                auto res = std::find(takenCompetitorIds.begin(), takenCompetitorIds.end(), competitorId->id);
                if (res != takenCompetitorIds.end())
                {
                    inUseCompetitors.push_back(object.index);
                }
            }
        }
        return inUseCompetitors;
    }

    // 0x00435AEF
    void aiDestroy(const CompanyId id)
    {
        auto* company = get(id);
        if (company == nullptr)
        {
            return;
        }
        if (company->headquartersX != -1)
        {
            GameCommands::HeadquarterRemovalArgs args{};
            args.pos = World::Pos3(company->headquartersX, company->headquartersY, company->headquartersZ * kSmallZStep);
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        Ui::WindowManager::close(Ui::WindowType::company, enumValue(id));
        Ui::WindowManager::close(Ui::WindowType::vehicleList, enumValue(id));
        Ui::WindowManager::close(Ui::WindowType::stationList, enumValue(id));
        Ui::WindowManager::close(Ui::WindowType::map);

        Ui::Windows::CompanyList::removeCompany(id);
        MessageManager::removeAllSubjectRefs(enumValue(id), MessageItemArgumentType::company);
        removeCompaniesRecords(id);
        StringManager::emptyUserString(company->name);
        company->name = StringIds::empty;
        StringManager::emptyUserString(company->ownerName);
        // TODO: Change this when we want to diverge from vanilla
        // company->ownerName = StringIds::empty;

        ObjectManager::unload(ObjectManager::getHeader(LoadedObjectHandle{
            ObjectType::competitor, company->competitorId }));
        ObjectManager::reloadAll();
        Ui::Dropdown::forceCloseCompanySelect();
    }

    // 0x0046E306
    currency32_t getInflationAdjustedStartingLoan()
    {
        return Economy::getInflationAdjustedCost(getStartingLoanSize(), 0, 8) / 100 * 100;
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
