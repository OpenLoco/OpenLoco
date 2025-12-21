#include "Scenario/ScenarioManager.h"
#include "EditorController.h"
#include "Environment.h"
#include "GameState.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Objects/CurrencyObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "Scenario/Scenario.h"
#include "Scenario/ScenarioOptions.h"
#include "SceneManager.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Stream.hpp>
#include <OpenLoco/Utility/String.hpp>
#include <fstream>

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
    };
    static_assert(sizeof(ScenarioFolderState) == 0xC);
    struct ScoreHeader
    {
        ScenarioFolderState state;
        uint32_t numScenarios; // Note this is a subset of state.numFiles
    };
    static_assert(sizeof(ScoreHeader) == 0x10);
#pragma pack(pop)

    // 0x0050AE8C
    static std::vector<ScenarioIndexEntry> _scenarioList;
    // 0x0050AE04
    static ScoreHeader _scenarioHeader;

    bool hasScenariosForCategory(uint8_t category)
    {
        for (const auto& entry : _scenarioList)
        {
            if (!entry.hasFlag(ScenarioIndexFlags::flag_0))
            {
                continue;
            }

            if (entry.category == category)
            {
                return true;
            }
        }

        return false;
    }

    bool hasScenarioInCategory(uint8_t category, ScenarioIndexEntry* scenario)
    {
        for (const auto& entry : _scenarioList)
        {
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
            {
                if (&entry == scenario)
                {
                    return false;
                }
                else
                {
                    continue;
                }
            }

            if (&entry == scenario)
            {
                return true;
            }
        }

        return false;
    }

    // 0x00443EF6, kind of
    uint16_t getScenarioCountByCategory(uint8_t category)
    {
        auto scenarioCountInCategory = 0;
        for (const auto& entry : _scenarioList)
        {
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
            {
                continue;
            }

            scenarioCountInCategory++;
        }

        return scenarioCountInCategory;
    }

    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index)
    {
        uint8_t j = 0;
        for (auto& entry : _scenarioList)
        {
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
            {
                continue;
            }

            if (j == index)
            {
                return &entry;
            }

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

        assert(_scenarioHeader.numScenarios == _scenarioList.size());
        stream.write(reinterpret_cast<const char*>(&_scenarioHeader), sizeof(ScoreHeader));
        stream.write(reinterpret_cast<const char*>(_scenarioList.data()), sizeof(ScenarioIndexEntry) * _scenarioList.size());
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
        Utility::readData(stream, _scenarioHeader);
        if (stream.gcount() != sizeof(ScoreHeader))
        {
            return false;
        }

        const auto scenarioListSize = _scenarioHeader.numScenarios * sizeof(ScenarioIndexEntry);
        _scenarioList.clear();
        _scenarioList.resize(_scenarioHeader.numScenarios);
        if (_scenarioList.empty())
        {
            exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
            return false;
        }
        Utility::readData(stream, _scenarioList.data(), _scenarioHeader.numScenarios);
        if (stream.gcount() != static_cast<int32_t>(scenarioListSize))
        {
            _scenarioHeader = ScoreHeader{};
            _scenarioList.clear();
            return false;
        }
        if (currentState != _scenarioHeader.state)
        {
            return false;
        }
        // version?
        if ((_scenarioHeader.state.numFiles >> 24) != 1)
        {
            return false;
        }

        return true;
    }

    static bool loadScenarioDetails(ScenarioIndexEntry& entry, Scenario::Options& options)
    {
        if ((options.scenarioText.flags & 0xFF) == 0xFF)
        {
            // Details are not loaded they are pre-populated
            strncpy(entry.scenarioName, options.scenarioName, sizeof(entry.scenarioName));
            strncpy(entry.description, options.scenarioDetails, sizeof(entry.description));
            return true;
        }

        if (!ObjectManager::loadTemporaryObject(options.scenarioText).has_value())
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
    static void loadScenarioProgress(ScenarioIndexEntry& entry, Scenario::Options& options)
    {
        const auto deliveredCargoObjTempLoaded = ObjectManager::loadTemporaryObject(options.objectiveDeliveredCargo);
        Scenario::Objective objective = options.objective;
        Scenario::ObjectiveProgress progress{};
        progress.timeLimitUntilYear = objective.timeLimitYears + options.scenarioStartYear - 1;
        if (deliveredCargoObjTempLoaded)
        {
            objective.deliveredCargoType = 0xFF; // Used to indicate formatChallengeArguments to use tempObj
        }
        std::optional<ObjectHeader> previousCurrency;
        if (ObjectManager::get<CurrencyObject>() != nullptr)
        {
            previousCurrency = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::currency, 0 });
            ObjectManager::unload(*previousCurrency);
        }
        ObjectManager::load(options.currency);
        Gfx::loadCurrency();

        FormatArguments args{};
        Scenario::formatChallengeArguments(objective, progress, args);

        FormatArgumentsView argsView{ args };
        const auto stringId = argsView.pop<StringId>();

        // TODO: Validate that argsView is copied in the current state.
        StringManager::formatString(entry.objective, stringId, argsView);

        ObjectManager::freeTemporaryObject();

        // Original didn't clean up like this
        if (previousCurrency)
        {
            ObjectManager::unload(options.currency);
            ObjectManager::load(*previousCurrency);
            Gfx::loadCurrency();
        }
    }

    static std::optional<uint32_t> findScenario(const fs::path& fileName)
    {
        const auto u8FileName = fileName.filename().u8string();
        auto res = std::ranges::find_if(_scenarioList, [&u8FileName](const ScenarioIndexEntry& entry) {
            return std::strcmp(entry.filename, u8FileName.c_str()) == 0;
        });
        if (res != _scenarioList.end())
        {
            return std::distance(_scenarioList.begin(), res);
        }
        return std::nullopt;
    }

    // 0x004447DF
    static void createIndex(const ScenarioFolderState& currentState)
    {
        Input::processMessagesMini();
        Ui::ProgressBar::begin(StringIds::checkingScenarioFiles);

        _scenarioHeader.state = currentState;
        _scenarioHeader.state.numFiles = (currentState.numFiles & 0xFFFFFF) | (1 << 24);

        for (auto& entry : _scenarioList)
        {
            entry.flags &= ~ScenarioIndexFlags::flag_0;
        }

        const auto scenarioPath = Environment::getPathNoWarning(Environment::PathId::scenarios);
        auto numScenariosDetected = 0;
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

            numScenariosDetected++;
        }

        auto currentScenarioOffset = 0;
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

            Input::processMessagesMini();

            currentScenarioOffset++;
            auto currentScenarioProgress = currentScenarioOffset * 225 / numScenariosDetected;
            Ui::ProgressBar::setProgress(currentScenarioProgress);

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
                // This is a new entry so we will need to clear fields and add to the list
                ScenarioIndexEntry entry{};
                std::strcpy(entry.filename, u8FileName.c_str());
                foundId = static_cast<uint32_t>(_scenarioList.size());
                _scenarioList.push_back(entry);
                _scenarioHeader.numScenarios++;
            }
            ScenarioIndexEntry& entry = _scenarioList[foundId.value()];

            entry.flags |= ScenarioIndexFlags::flag_0;
            entry.category = options->difficulty;
            entry.flags &= ~ScenarioIndexFlags::hasPreviewImage;
            if ((options->scenarioFlags & Scenario::ScenarioFlags::landscapeGenerationDone) != Scenario::ScenarioFlags::none)
            {
                entry.flags |= ScenarioIndexFlags::hasPreviewImage;
                std::copy(&options->preview[0][0], &options->preview[0][0] + sizeof(options->preview), &entry.preview[0][0]);
            }
            entry.startYear = options->scenarioStartYear;
            entry.numCompetingCompanies = options->maxCompetingCompanies;
            entry.competingCompanyDelay = options->competitorStartDelay;
            entry.currency = options->currency;

            loadScenarioProgress(entry, *options);
            loadScenarioDetails(entry, *options);
        }

        std::ranges::sort(_scenarioList, [](const ScenarioIndexEntry& lhs, const ScenarioIndexEntry& rhs) {
            return strcmp(lhs.scenarioName, rhs.scenarioName) < 0;
        });

        Ui::ProgressBar::setProgress(230);
        saveIndex();
        Ui::ProgressBar::setProgress(240);
        ObjectManager::reloadAll();
        Ui::ProgressBar::end();
    }

    // 0x0044452F
    void loadIndex(const bool forceReload)
    {
        const auto oldFlags = SceneManager::getSceneFlags();
        SceneManager::setSceneFlags(SceneManager::Flags::title | oldFlags);

        const auto currentState = getCurrentScenarioFolderState();
        // We should always try and load as we want to keep scores
        // when adding/removing scenarios
        if (!tryLoadIndex(currentState) || forceReload)
        {
            createIndex(currentState);
        }

        setSceneFlags(oldFlags);
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
