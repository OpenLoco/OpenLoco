#include "Config.h"
#include "ConfigConvert.hpp"
#include "Environment.h"
#include <Message.h>
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <fstream>
#include <locale>
#include <yaml-cpp/yaml.h>

namespace OpenLoco::Config
{
    static Config _config;
    static YAML::Node _configYaml;

    constexpr ObjectHeader kDefaultPreferredCurrency = { 0x00000082u, { 'C', 'U', 'R', 'R', 'D', 'O', 'L', 'L' }, 0u };

    Config& get()
    {
        return _config;
    }

    static void readShortcutConfig(const YAML::Node& scNode)
    {
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto& shortcuts = _config.shortcuts;
        for (const auto& def : shortcutDefs)
        {
            auto node = scNode[def.configName];
            if (node)
            {
                shortcuts[def.id] = node.as<KeyboardShortcut>();
            }
            else
            {
                shortcuts[def.id] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
            }
        }
    }

    Config& read()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::openlocoYML);

        // No config file? Use defaults.
        if (!fs::exists(configPath))
        {
            readShortcutConfig(YAML::Node());
            return _config;
        }

        // On Windows, YAML::LoadFile only supports ANSI paths, so we pass an ifstream instead.
        std::ifstream stream;
        stream.exceptions(std::ifstream::failbit);
        stream.open(configPath, std::ios::in | std::ios::binary);
        _configYaml = YAML::Load(stream);

        const auto& config = _configYaml;

        // Display settings
        auto& displayNode = config["display"];
        if (displayNode && displayNode.IsMap())
        {
            auto& displayConfig = _config.display;
            displayConfig.mode = displayNode["mode"].as<ScreenMode>(ScreenMode::window);
            displayConfig.index = displayNode["index"].as<int32_t>(0);
            displayConfig.windowResolution = displayNode["window_resolution"].as<Resolution>(Resolution{ 800, 600 });
            displayConfig.fullscreenResolution = displayNode["fullscreen_resolution"].as<Resolution>(Resolution{ 1920, 1080 });
            displayConfig.vsync = displayNode["vsync"].as<bool>(false);
        }

        // Audio settings
        auto& audioNode = config["audio"];
        if (audioNode && audioNode.IsMap())
        {
            auto& audioConfig = _config.audio;
            audioConfig.device = audioNode["device"].as<std::string>("");
            audioConfig.mainVolume = audioNode["mainVolume"].as<int32_t>(-1100);
            audioConfig.playJukeboxMusic = audioNode["playJukeboxMusic"].as<bool>(true);
            audioConfig.playTitleMusic = audioNode["play_title_music"].as<bool>(true);
            audioConfig.playNewsSounds = audioNode["play_news_sounds"].as<bool>(true);
            audioConfig.playlist = audioNode["playlist"].as<MusicPlaylistType>(MusicPlaylistType::currentEra);

            if (audioNode["customJukebox"])
            {
                audioConfig.customJukebox = audioNode["customJukebox"].as<Playlist>(Playlist{});
            }
            else
            {
                std::fill(audioConfig.customJukebox.begin(), audioConfig.customJukebox.end(), true);
            }
        }

        // Network settings
        auto& networkNode = config["network"];
        if (networkNode && networkNode.IsMap())
        {
            auto& networkConfig = _config.network;
            networkConfig.enabled = networkNode["enabled"].as<bool>(false);
        }

        // General
        _config.locoInstallPath = config["loco_install_path"].as<std::string>("");
        _config.lastSavePath = config["last_save_path"].as<std::string>("");
        _config.lastLandscapePath = config["last_landscape_path"].as<std::string>("");

        // Regional
        _config.language = config["language"].as<std::string>("en-GB");
        _config.measurementFormat = config["measurementFormat"].as<MeasurementFormat>(MeasurementFormat::imperial);
        _config.preferredCurrency = config["preferredCurrency"].as<ObjectHeader>(kDefaultPreferredCurrency);
        _config.usePreferredCurrencyForNewGames = config["usePreferredCurrencyForNewGames"].as<bool>(false);
        _config.usePreferredCurrencyAlways = config["usePreferredCurrencyAlways"].as<bool>(false);

        // Display
        _config.scaleFactor = config["scale_factor"].as<float>(1.0f);
        _config.showFPS = config["showFPS"].as<bool>(false);
        _config.uncapFPS = config["uncapFPS"].as<bool>(false);

        // Rendering
        _config.constructionMarker = config["constructionMarker"].as<int32_t>(0);
        _config.gridlinesOnLandscape = config["gridlinesOnLandscape"].as<bool>(false);
        _config.heightMarkerOffset = config["heightMarkerOffset"].as<int32_t>(1);
        _config.landscapeSmoothing = config["landscapeSmoothing"].as<bool>(true);
        _config.showHeightAsUnits = config["showHeightAsUnits"].as<bool>(false);
        _config.stationNamesMinScale = config["stationNamesMinScale"].as<int32_t>(2);
        _config.vehiclesMinScale = config["vehiclesMinScale"].as<int32_t>(2);

        // News settings
        auto& newsNode = config["news"];
        auto& newsConfig = _config.newsSettings;
        if (newsNode && newsNode.IsMap())
        {
            newsConfig[enumValue(MessageCriticality::majorCompany)] = newsNode["majorCompany"].as<NewsType>(NewsType::newsWindow);
            newsConfig[enumValue(MessageCriticality::majorCompetitor)] = newsNode["majorCompetitor"].as<NewsType>(NewsType::newsWindow);
            newsConfig[enumValue(MessageCriticality::minorCompany)] = newsNode["minorCompany"].as<NewsType>(NewsType::newsWindow);
            newsConfig[enumValue(MessageCriticality::minorCompetitor)] = newsNode["minorCompetitor"].as<NewsType>(NewsType::newsWindow);
            newsConfig[enumValue(MessageCriticality::general)] = newsNode["general"].as<NewsType>(NewsType::newsWindow);
            newsConfig[enumValue(MessageCriticality::advice)] = newsNode["advice"].as<NewsType>(NewsType::newsWindow);
        }
        else
        {
            newsConfig[enumValue(MessageCriticality::majorCompany)] = NewsType::newsWindow;
            newsConfig[enumValue(MessageCriticality::majorCompetitor)] = NewsType::newsWindow;
            newsConfig[enumValue(MessageCriticality::minorCompany)] = NewsType::newsWindow;
            newsConfig[enumValue(MessageCriticality::minorCompetitor)] = NewsType::newsWindow;
            newsConfig[enumValue(MessageCriticality::general)] = NewsType::newsWindow;
            newsConfig[enumValue(MessageCriticality::advice)] = NewsType::newsWindow;
        }

        // General UI
        _config.allowMultipleInstances = config["allow_multiple_instances"].as<bool>(false);
        _config.cashPopupRendering = config["cashPopupRendering"].as<bool>(true);
        _config.edgeScrolling = config["edgeScrolling"].as<bool>(true);
        _config.edgeScrollingSpeed = config["edgeScrollingSpeed"].as<int32_t>(12);
        _config.windowFrameStyle = config["windowFrameStyle"].as<WindowFrameStyle>(WindowFrameStyle::background);
        _config.zoomToCursor = config["zoom_to_cursor"].as<bool>(true);

        // Saving and autosaves
        _config.autosaveAmount = config["autosave_amount"].as<int32_t>(12);
        _config.autosaveFrequency = config["autosave_frequency"].as<int32_t>(1);
        _config.exportObjectsWithSaves = config["exportObjectsWithSaves"].as<bool>(true);

        // Cheats
        _config.breakdownsDisabled = config["breakdowns_disabled"].as<bool>(false);
        _config.buildLockedVehicles = config["buildLockedVehicles"].as<bool>(false);
        _config.cheatsMenuEnabled = config["cheats_menu_enabled"].as<bool>(false);
        _config.companyAIDisabled = config["companyAIDisabled"].as<bool>(false);
        _config.disableVehicleLoadPenaltyCheat = config["disableVehicleLoadPenaltyCheat"].as<bool>(false);
        _config.displayLockedVehicles = config["displayLockedVehicles"].as<bool>(false);
        _config.invertRightMouseViewPan = config["invertRightMouseViewPan"].as<bool>(false);
        _config.townGrowthDisabled = config["townGrowthDisabled"].as<bool>(false);
        _config.trainsReverseAtSignals = config["trainsReverseAtSignals"].as<bool>(false);
        _config.disableStationSizeLimit = config["disableStationSizeLimit"].as<bool>(false);
        _config.showAiPlanningAsGhosts = config["showAiPlanningAsGhosts"].as<bool>(false);
        _config.keepCargoModifyPickup = config["keepCargoModifyPickup"].as<bool>(false);

        // Preferred owner
        _config.preferredOwnerName = config["preferredOwnerName"].as<std::string>("");
        _config.usePreferredOwnerName = config["usePreferredOwnerName"].as<bool>(false);
        _config.preferredOwnerFace = config["preferredOwnerFace"].as<ObjectHeader>(kEmptyObjectHeader);
        _config.usePreferredOwnerFace = config["usePreferredOwnerFace"].as<bool>(false);

        // Misc settings
        _config.scenarioSelectedTab = config["scenarioSelectedTab"].as<int32_t>(2);

        // Shortcuts
        auto& scNode = config["shortcuts"];
        // Protect from empty shortcuts
        readShortcutConfig(scNode ? scNode : YAML::Node{});

        return _config;
    }

    void write()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::openlocoYML);
        auto dir = configPath.parent_path();
        Environment::autoCreateDirectory(dir);

        auto backupLocale = std::locale::global(std::locale::classic());
        auto& node = _configYaml;

        // Display
        const auto& displayConfig = _config.display;
        auto displayNode = node["display"];
        displayNode["mode"] = displayConfig.mode;
        if (displayConfig.index != 0)
        {
            displayNode["index"] = displayConfig.index;
        }
        else
        {
            displayNode.remove("index");
        }
        displayNode["window_resolution"] = displayConfig.windowResolution;
        displayNode["fullscreen_resolution"] = displayConfig.fullscreenResolution;
        displayNode["vsync"] = displayConfig.vsync;
        node["display"] = displayNode;

        // Audio
        const auto& audioConfig = _config.audio;
        auto audioNode = node["audio"];
        audioNode["device"] = audioConfig.device;
        if (audioConfig.device.empty())
        {
            audioNode.remove("device");
        }
        audioNode["mainVolume"] = audioConfig.mainVolume;
        audioNode["playJukeboxMusic"] = audioConfig.playJukeboxMusic;
        audioNode["play_title_music"] = audioConfig.playTitleMusic;
        audioNode["playNewsSounds"] = audioConfig.playNewsSounds;
        audioNode["playlist"] = audioConfig.playlist;
        audioNode["customJukebox"] = audioConfig.customJukebox;
        node["audio"] = audioNode;

        // Network
        const auto& networkConfig = _config.network;
        auto networkNode = node["network"];
        networkNode["enabled"] = networkConfig.enabled;
        node["network"] = networkNode;

        // General
        node["loco_install_path"] = _config.locoInstallPath;
        node["last_save_path"] = _config.lastSavePath;
        node["last_landscape_path"] = _config.lastLandscapePath;

        // Regional
        node["language"] = _config.language;
        node["measurementFormat"] = _config.measurementFormat;
        node["preferredCurrency"] = _config.preferredCurrency;
        node["usePreferredCurrencyForNewGames"] = _config.usePreferredCurrencyForNewGames;
        node["usePreferredCurrencyAlways"] = _config.usePreferredCurrencyAlways;

        // Display
        node["scale_factor"] = _config.scaleFactor;
        node["showFPS"] = _config.showFPS;
        node["uncapFPS"] = _config.uncapFPS;

        // Rendering
        node["constructionMarker"] = _config.constructionMarker;
        node["gridlinesOnLandscape"] = _config.gridlinesOnLandscape;
        node["heightMarkerOffset"] = _config.heightMarkerOffset;
        node["showHeightAsUnits"] = _config.showHeightAsUnits;
        node["landscapeSmoothing"] = _config.landscapeSmoothing;
        node["vehiclesMinScale"] = _config.vehiclesMinScale;
        node["stationNamesMinScale"] = _config.stationNamesMinScale;

        // News settings
        const auto& newsConfig = _config.newsSettings;
        auto newsNode = node["news"];
        newsNode["majorCompany"] = newsConfig[enumValue(MessageCriticality::majorCompany)];
        newsNode["majorCompetitor"] = newsConfig[enumValue(MessageCriticality::majorCompetitor)];
        newsNode["minorCompany"] = newsConfig[enumValue(MessageCriticality::minorCompany)];
        newsNode["minorCompetitor"] = newsConfig[enumValue(MessageCriticality::minorCompetitor)];
        newsNode["general"] = newsConfig[enumValue(MessageCriticality::general)];
        newsNode["advice"] = newsConfig[enumValue(MessageCriticality::advice)];
        node["news"] = newsNode;

        // General UI
        node["allow_multiple_instances"] = _config.allowMultipleInstances;
        node["cashPopupRendering"] = _config.cashPopupRendering;
        node["edgeScrolling"] = _config.edgeScrolling;
        node["edgeScrollingSpeed"] = _config.edgeScrollingSpeed;
        node["windowFrameStyle"] = _config.windowFrameStyle;
        node["zoom_to_cursor"] = _config.zoomToCursor;

        // Saving and autosaves
        node["autosave_amount"] = _config.autosaveAmount;
        node["autosave_frequency"] = _config.autosaveFrequency;
        node["exportObjectsWithSaves"] = _config.exportObjectsWithSaves;

        // Cheats
        node["breakdowns_disabled"] = _config.breakdownsDisabled;
        node["buildLockedVehicles"] = _config.buildLockedVehicles;
        node["cheats_menu_enabled"] = _config.cheatsMenuEnabled;
        node["companyAIDisabled"] = _config.companyAIDisabled;
        node["disableVehicleLoadPenaltyCheat"] = _config.disableVehicleLoadPenaltyCheat;
        node["displayLockedVehicles"] = _config.displayLockedVehicles;
        node["invertRightMouseViewPan"] = _config.invertRightMouseViewPan;
        node["townGrowthDisabled"] = _config.townGrowthDisabled;
        node["trainsReverseAtSignals"] = _config.trainsReverseAtSignals;
        node["disableStationSizeLimit"] = _config.disableStationSizeLimit;
        node["showAiPlanningAsGhosts"] = _config.showAiPlanningAsGhosts;
        node["keepCargoModifyPickup"] = _config.keepCargoModifyPickup;

        // Preferred owner
        node["preferredOwnerName"] = _config.preferredOwnerName;
        node["usePreferredOwnerName"] = _config.usePreferredOwnerName;
        node["preferredOwnerFace"] = _config.preferredOwnerFace;
        node["usePreferredOwnerFace"] = _config.usePreferredOwnerFace;

        // Shortcuts
        const auto& shortcuts = _config.shortcuts;
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto scNode = node["shortcuts"];
        for (const auto& def : shortcutDefs)
        {
            auto it = shortcuts.find(def.id);
            if (it != std::end(shortcuts))
            {
                scNode[def.configName] = it->second;
            }
            else
            {
                scNode[def.configName] = "";
            }
        }
        node["shortcuts"] = scNode;

        // Misc settings
        node["scenarioSelectedTab"] = _config.scenarioSelectedTab;

        std::ofstream stream(configPath);
        if (stream.is_open())
        {
            stream << node << std::endl;
        }

        std::locale::global(backupLocale);
    }

    // 0x004BE3F3
    void resetShortcuts()
    {
        const auto& shortcutDefs = Input::ShortcutManager::getList();

        auto& shortcuts = _config.shortcuts;
        shortcuts.clear();

        for (const auto& def : shortcutDefs)
        {
            shortcuts[def.id] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
        }

        write();
    }
}
