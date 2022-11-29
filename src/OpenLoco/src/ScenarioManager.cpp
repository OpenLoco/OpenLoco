#include "ScenarioManager.h"
#include "CompanyManager.h"
#include "EditorController.h"
#include "Environment.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Objects/CurrencyObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "Ui.h"
#include <OpenLoco/Utility/Stream.hpp>
#include <fstream>

using namespace OpenLoco::Interop;

namespace OpenLoco::ScenarioManager
{
#pragma pack(push, 1)
    struct ScenarioFolderState
    {
        uint32_t numFiles = 0;
        uint64_t totalFileSize = 0;
        constexpr bool operator==(const ScenarioFolderState& rhs) const
        {
            return ((numFiles & 0xFFFFFF) == (rhs.numFiles & 0xFFFFFF)) && (totalFileSize == rhs.totalFileSize);
        }
        constexpr bool operator!=(const ScenarioFolderState& rhs) const
        {
            return !(*this == rhs);
        }
    };
    static_assert(sizeof(ScenarioFolderState) == 0xC);
    struct ScoreHeader
    {
        ScenarioFolderState state;
        uint32_t numScenarios; // Note this is a subset of state.numFiles
    };
    static_assert(sizeof(ScoreHeader) == 0x10);
#pragma pack(pop)

    loco_global<ScenarioIndexEntry*, 0x0050AE8C> _scenarioList;
    loco_global<ScoreHeader, 0x0050AE04> _scenarioHeader;

    bool hasScenariosForCategory(uint8_t category)
    {
        for (uint32_t i = 0; i < _scenarioHeader->numScenarios; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            if (!entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            if (entry.category == category)
                return true;
        }

        return false;
    }

    bool hasScenarioInCategory(uint8_t category, ScenarioIndexEntry* scenario)
    {
        for (uint32_t i = 0; i < _scenarioHeader->numScenarios; i++)
        {
            ScenarioIndexEntry* entry = &_scenarioList[i];

            if (entry->category != category || !entry->hasFlag(ScenarioIndexFlags::flag_0))
            {
                if (entry == scenario)
                    return false;
                else
                    continue;
            }

            if (entry == scenario)
                return true;
        }

        return false;
    }

    // 0x00443EF6, kind of
    uint16_t getScenarioCountByCategory(uint8_t category)
    {
        auto scenarioCountInCategory = 0;
        for (uint32_t i = 0; i < _scenarioHeader->numScenarios; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            scenarioCountInCategory++;
        }

        return scenarioCountInCategory;
    }

    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index)
    {
        uint8_t j = 0;
        for (uint32_t i = 0; i < _scenarioHeader->numScenarios; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            if (j == index)
                return &entry;

            j++;
        }

        return nullptr;
    }

    static void saveIndex() {

    }

    // 0x00444574
    static ScenarioFolderState getCurrentScenarioFolderState()
    {
        ScenarioFolderState currentState;
        const auto scenarioPath = Environment::getPathNoWarning(Environment::PathId::scenarios);
        for (const auto& file : fs::directory_iterator(scenarioPath, fs::directory_options::skip_permission_denied))
        {
            if (!file.is_regular_file())
            {
                continue;
            }
            currentState.numFiles++;
            currentState.totalFileSize += file.file_size();
        }
        return currentState;
    }

    // 0x00444611
    static bool tryLoadIndex(const ScenarioFolderState& currentState)
    {
        const auto scorePath = Environment::getPath(Environment::PathId::scores);
        if (!fs::exists(scorePath))
        {
            return false;
        }
        std::ifstream stream;
        stream.open(scorePath, std::ios::in | std::ios::binary);
        if (!stream.is_open())
        {
            return false;
        }
        // 0x00112A14C -> 160
        _scenarioHeader = ScoreHeader{};
        Utility::readData(stream, *_scenarioHeader);
        if (stream.gcount() != sizeof(ScoreHeader))
        {
            return false;
        }

        const auto scenarioListSize = _scenarioHeader->numScenarios * sizeof(ScenarioIndexEntry);
        _scenarioList = static_cast<ScenarioIndexEntry*>(malloc(scenarioListSize));
        if (_scenarioList == nullptr)
        {
            exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
            return false;
        }
        Utility::readData(stream, *_scenarioList, _scenarioHeader->numScenarios);
        if (stream.gcount() != static_cast<int32_t>(scenarioListSize))
        {
            _scenarioHeader = ScoreHeader{};
            free(_scenarioList);
            return false;
        }
        if (currentState != _scenarioHeader->state)
        {
            return false;
        }
        // version?
        if ((_scenarioHeader->state.numFiles >> 24) != 1)
        {
            return false;
        }

        return true;
    }

    static bool loadScenarioDetails(ScenarioIndexEntry& entry, S5::Options& options)
    {
        if ((options.scenarioText.flags & 0xFF) == 0xFF)
        {
            // Details are not loaded they are prepopulated
            return true;
        }

        if (!ObjectManager::loadTemporaryObject(options.scenarioText))
        {
            return false;
        }

        auto* stex = reinterpret_cast<ScenarioTextObject*>(ObjectManager::getTemporaryObject());
        StringManager::formatString(entry.scenarioName, stex->name);
        StringManager::formatString(entry.description, stex->details);
        ObjectManager::freeTemporaryObject();
        return true;
    }

    // 0x00444C4E
    static void loadScenarioProgress(ScenarioIndexEntry& entry, S5::Options& options)
    {
        ObjectManager::loadTemporaryObject(options.objectiveDeliveredCargo);
        Scenario::Objective objective = options.objective;
        Scenario::ObjectiveProgress progress{};
        progress.timeLimitUntilYear = objective.timeLimitYears + options.scenarioStartYear - 1;
        objective.deliveredCargoType = 0xFF; // Used to indicate formatChallengeArguments to use tempObj

        std::optional<ObjectHeader> previousCurrency;
        if (ObjectManager::get<CurrencyObject>() != nullptr)
        {
            previousCurrency = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::currency, 0 });
            ObjectManager::unload(*previousCurrency);
        }
        ObjectManager::load(options.currency);
        ObjectManager::reloadAll();
        call(0x0046E07B); // load currency gfx

        FormatArguments args{};
        Scenario::formatChallengeArguments(objective, progress, args);
        StringManager::formatString(entry.objective, *reinterpret_cast<const string_id*>(&args), reinterpret_cast<const std::byte*>(&args) + sizeof(string_id));

        ObjectManager::freeTemporaryObject();

        // Original didn't clean up like this
        if (previousCurrency)
        {
            ObjectManager::unload(options.currency);
            ObjectManager::load(*previousCurrency);
            ObjectManager::reloadAll();
            call(0x0046E07B); // load currency gfx
        }
    }

    // 0x004447DF
    static void createIndex(const ScenarioFolderState& currentState)
    {
        if (_scenarioList == reinterpret_cast<ScenarioIndexEntry*>(-1) || _scenarioList == nullptr)
        {
            _scenarioHeader->numScenarios = 0;
            _scenarioList = static_cast<ScenarioIndexEntry*>(malloc(currentState.numFiles * sizeof(ScenarioIndexEntry)));
            if (_scenarioList == nullptr)
            {
                exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
                return;
            }
            std::fill(*_scenarioList, *_scenarioList + currentState.numFiles, ScenarioIndexEntry{});
        }
        _scenarioHeader->state = currentState;
        _scenarioHeader->state.numFiles = (currentState.numFiles & 0xFFFFFF) | (1 << 24);

        for (uint32_t i = 0; i < _scenarioHeader->numScenarios; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            entry.flags &= ~ScenarioIndexFlags::flag_0;
        }

        const auto scenarioPath = Environment::getPathNoWarning(Environment::PathId::scenarios);
        for (const auto& file : fs::directory_iterator(scenarioPath, fs::directory_options::skip_permission_denied))
        {
            if (!file.is_regular_file())
            {
                continue;
            }
            Ui::processMessagesMini();

            const auto u8FileName = file.path().filename().u8string();
            bool entryFound = false;
            uint32_t foundId = 0;
            for (; foundId < _scenarioHeader->numScenarios; foundId++)
            {
                ScenarioIndexEntry& entry = _scenarioList[foundId];

                if (std::strcmp(entry.filename, u8FileName.c_str()) == 0)
                {
                    entryFound = true;
                    break;
                }
            }

            const auto options = S5::readScenarioOptions(file.path());
            if (options == nullptr)
            {
                continue;
            }
            if (options->editorStep != EditorController::Step::null)
            {
                continue;
            }
            ScenarioIndexEntry& entry = _scenarioList[entryFound ? foundId : _scenarioHeader->numScenarios];

            entry.flags |= ScenarioIndexFlags::flag_0;
            entry.category = options->difficulty;
            entry.flags &= ~hasPreviewImage;
            if (options->scenarioFlags & Scenario::Flags::landscapeGenerationDone)
            {
                entry.flags |= hasPreviewImage;
                std::copy(&options->preview[0][0], &options->preview[0][0] + sizeof(options->preview), &entry.preview[0][0]);
            }
            entry.startYear = options->scenarioStartYear;
            entry.numCompetingCompanies = options->maxCompetingCompanies;
            entry.competingCompanyDelay = options->competitorStartDelay;

            entry.currency = options->currency;
            loadScenarioProgress(entry, *options);
            if (entryFound)
            {
            }
            else
            {
                _scenarioHeader->numScenarios++;
                std::strcpy(entry.filename, u8FileName.c_str());
            }
            loadScenarioDetails(entry, *options);
        }

        std::sort(*_scenarioList, *_scenarioList + _scenarioHeader->numScenarios, [](const ScenarioIndexEntry& lhs, const ScenarioIndexEntry& rhs) {
            return strcmp(lhs.scenarioName, rhs.scenarioName) < 0;
            });
        saveIndex();
    }

    // 0x0044452F
    void loadIndex(uint8_t al)
    {
        const auto oldFlags = getScreenFlags();
        setAllScreenFlags(ScreenFlags::title | oldFlags);

        if (_scenarioList != reinterpret_cast<ScenarioIndexEntry*>(-1) && _scenarioList != nullptr)
        {
            free(_scenarioList);
        }

        const auto currentState = getCurrentScenarioFolderState();

        if (!tryLoadIndex(currentState))
        {
            createIndex(currentState);
        }
        else
        {
            if (al != 0)
            {
                createIndex(currentState);
            }
        }
        setAllScreenFlags(oldFlags);
    }

    // 0x00438959
    void saveNewScore(Scenario::ObjectiveProgress& progress, const CompanyId companyId)
    {
        auto* company = CompanyManager::get(companyId);
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00438959, regs);
    }

    // 0x00525F5E
    uint32_t getScenarioTicks()
    {
        return getGameState().scenarioTicks;
    }
    void setScenarioTicks(uint32_t ticks)
    {
        getGameState().scenarioTicks = ticks;
    }

    // 0x00525F64
    uint32_t getScenarioTicks2()
    {
        return getGameState().scenarioTicks2;
    }
    void setScenarioTicks2(uint32_t ticks)
    {
        getGameState().scenarioTicks2 = ticks;
    }
}
