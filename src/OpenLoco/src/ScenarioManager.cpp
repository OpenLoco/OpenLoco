#include "ScenarioManager.h"
#include "EditorController.h"
#include "Environment.h"
#include "GameState.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Objects/CurrencyObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "Ui.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/Stream.hpp>
#include <OpenLoco/Utility/String.hpp>
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

    // 0x00444B61
    static void saveIndex()
    {
        const auto scorePath = Environment::getPath(Environment::PathId::scores);
        std::ofstream stream;
        stream.open(scorePath, std::ios::out | std::ios::binary);
        if (!stream.is_open())
        {
            return;
        }

        stream.write(reinterpret_cast<const char*>(&*_scenarioHeader), sizeof(ScoreHeader));
        stream.write(reinterpret_cast<const char*>(&_scenarioList[0]), sizeof(ScenarioIndexEntry) * _scenarioHeader->numScenarios);
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
            _scenarioList = nullptr;
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
            strncpy(entry.scenarioName, options.scenarioName, sizeof(entry.scenarioName));
            strncpy(entry.description, options.scenarioDetails, sizeof(entry.description));
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
        Gfx::loadCurrency();

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
            Gfx::loadCurrency();
        }
    }

    static std::optional<uint32_t> findScenario(const fs::path& fileName)
    {
        const auto u8FileName = fileName.filename().u8string();
        const auto* begin = &_scenarioList[0];
        const auto* end = &_scenarioList[_scenarioHeader->numScenarios];
        auto res = std::find_if(begin, end, [&u8FileName](const ScenarioIndexEntry& entry) {
            return std::strcmp(entry.filename, u8FileName.c_str()) == 0;
        });
        if (res != end)
        {
            return std::distance(begin, res);
        }
        return std::nullopt;
    }

    // 0x004447DF
    static void createIndex(const ScenarioFolderState& currentState)
    {
        auto indexAllocSize = _scenarioHeader->numScenarios;
        if (_scenarioList == reinterpret_cast<ScenarioIndexEntry*>(-1) || _scenarioList == nullptr)
        {
            _scenarioHeader->numScenarios = 0;
            indexAllocSize = currentState.numFiles;
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
            if (!Utility::iequals(file.path().extension().u8string(), ".sc5"))
            {
                continue;
            }
            Ui::processMessagesMini();

            const auto u8FileName = file.path().filename().u8string();
            auto foundId = findScenario(u8FileName);

            const auto options = S5::readScenarioOptions(file.path());
            if (options == nullptr)
            {
                continue;
            }
            if (options->editorStep != EditorController::Step::null)
            {
                continue;
            }
            if (!foundId.has_value())
            {
                // Its possible to have more scenarios than files in the scenario folder
                // this is because even deleted scenarios need to keep their scores entry
                // due to this we will realloc if we get to this point. Also when adding
                // a scenario to the folder you will need to realloc as it has alloced only,
                // enough space for the previous amount of scenarios.
                // TODO: Use a vector after all free/mallocs of _scenarioList implemented
                if (_scenarioHeader->numScenarios >= indexAllocSize)
                {
                    const auto clearFromIndex = _scenarioHeader->numScenarios;
                    indexAllocSize = _scenarioHeader->numScenarios + 1;
                    _scenarioList = static_cast<ScenarioIndexEntry*>(realloc(_scenarioList, indexAllocSize * sizeof(ScenarioIndexEntry)));
                    // Zero the new entries
                    std::fill_n(&_scenarioList[clearFromIndex], indexAllocSize - clearFromIndex, ScenarioIndexEntry{});
                }
            }
            ScenarioIndexEntry& entry = _scenarioList[foundId.value_or(_scenarioHeader->numScenarios)];

            entry.flags |= ScenarioIndexFlags::flag_0;
            entry.category = options->difficulty;
            entry.flags &= ~hasPreviewImage;
            if ((options->scenarioFlags & Scenario::ScenarioFlags::landscapeGenerationDone) != Scenario::ScenarioFlags::none)
            {
                entry.flags |= hasPreviewImage;
                std::copy(&options->preview[0][0], &options->preview[0][0] + sizeof(options->preview), &entry.preview[0][0]);
            }
            entry.startYear = options->scenarioStartYear;
            entry.numCompetingCompanies = options->maxCompetingCompanies;
            entry.competingCompanyDelay = options->competitorStartDelay;

            entry.currency = options->currency;
            loadScenarioProgress(entry, *options);
            if (!foundId.has_value())
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
    void loadIndex(const bool forceReload)
    {
        const auto oldFlags = getScreenFlags();
        setAllScreenFlags(ScreenFlags::title | oldFlags);

        if (_scenarioList != reinterpret_cast<ScenarioIndexEntry*>(-1) && _scenarioList != nullptr)
        {
            free(_scenarioList);
            _scenarioList = nullptr;
        }

        const auto currentState = getCurrentScenarioFolderState();
        if (forceReload || !tryLoadIndex(currentState))
        {
            createIndex(currentState);
        }

        setAllScreenFlags(oldFlags);
    }

    // 0x00438959
    void saveNewScore(Scenario::ObjectiveProgress& progress, const CompanyId companyId)
    {
        auto res = findScenario(getGameState().scenarioFileName);
        if (!res.has_value())
        {
            return;
        }
        auto& entry = _scenarioList[res.value()];
        if (!entry.hasFlag(ScenarioIndexFlags::completed) || progress.completedChallengeInMonths < entry.completedMonths)
        {
            entry.flags |= ScenarioIndexFlags::completed;
            entry.completedMonths = progress.completedChallengeInMonths;
            auto* company = CompanyManager::get(companyId);
            StringManager::formatString(entry.highscoreName, company->name);
            saveIndex();
        }
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
