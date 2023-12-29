#include "Config.h"
#include "ConfigConvert.hpp"
#include "Environment.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <fstream>
#include <yaml-cpp/yaml.h>

using namespace OpenLoco::Interop;

namespace OpenLoco::Config
{
    static loco_global<LocoConfig, 0x0050AEB4> _legacyConfig;
    static loco_global<uint8_t, 0x0050AEAD> _50AEAD;
    static loco_global<uint32_t, 0x0113E21C> _totalPhysicalMemory;
    static NewConfig _newConfig;
    static YAML::Node _configYaml;

    static LocoConfig& getLegacy()
    {
        return _legacyConfig;
    }

    NewConfig& get()
    {
        return _newConfig;
    }

    constexpr ObjectHeader kDefaultPreferredCurrency = { 0x00000082u, { 'C', 'U', 'R', 'R', 'D', 'O', 'L', 'L' }, 0u };

    constexpr LocoConfig kDefaultConfig = {
        /*flags*/ Flags::exportObjectsWithSaves,
        /*resolutionWidth*/ -1,
        /*resolutionHeight*/ -1,
        /*backupResolutionWidth*/ 0xFFFFU,
        /*backupResolutionHeight*/ 0xFFFFU,
        /*countdown*/ 0,
        /*var_0D*/ false,
        /*audioDeviceGuid*/ { 0 },
        /*var_1E*/ 1,
        /*forceSoftwareAudioMixer*/ 0,
        /*musicPlaying*/ 1,
        /*constructionMarker*/ 0,
        /*maxVehicleSounds*/ 16,
        /*maxSoundInstances*/ 10,
        /*soundQuality*/ 2,
        /*measurementFormat*/ MeasurementFormat::imperial,
        /*pad_29*/ 1,
        /*keyboardShortcuts*/ {
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
            { 0xFF, 0xFF },
        },
        /*edgeScrolling*/ 1,
        /*vehiclesMinScale*/ 2,
        /*var_72*/ 0,
        /*musicPlaylist*/ MusicPlaylistType::currentEra,
        /*heightMarkerOffset*/ 0x100,
        /*newsSettings*/ { NewsType::newsWindow, NewsType::newsWindow, NewsType::ticker, NewsType::newsWindow, NewsType::newsWindow },
        /*preferredCurrency*/ kDefaultPreferredCurrency,
        /*enabledMusic*/ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        /*pad_A9*/ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        /*volume*/ -1100,
        /*connectionTimeout*/ 25000,
        /*lastHost*/ { 0 },
        /*stationNamesMinScale*/ 2,
        /*scenarioSelectedTab*/ 0,
        /*preferredName*/ { 0 },
    };

    constexpr uint8_t kDefaultMaxVehicleSounds[3] = { 4, 8, 16 };
    constexpr uint8_t kDefaultMaxSoundInstances[3] = { 6, 8, 10 };
    constexpr uint32_t kLegacyConfigMagicNumber = 0x62272;

    static void writeNewConfig();

    static void setDefaultsLegacyConfig()
    {
        getLegacy() = kDefaultConfig;
        if (_totalPhysicalMemory <= (64 * 1024 * 1024)) // 64 MB
        {
            getLegacy().soundQuality = 0;
            getLegacy().vehiclesMinScale = 1;
        }
        else if (_totalPhysicalMemory <= 128 * 1024 * 1024) // 128 MB
        {
            getLegacy().soundQuality = 1;
            getLegacy().vehiclesMinScale = 1;
        }
        else
        {
            getLegacy().soundQuality = 2;
            getLegacy().vehiclesMinScale = 2;
        }
        getLegacy().maxVehicleSounds = kDefaultMaxVehicleSounds[getLegacy().soundQuality];
        getLegacy().maxSoundInstances = kDefaultMaxSoundInstances[getLegacy().soundQuality];
    }

    // 0x00441A6C
    static LocoConfig& readLegacy()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::gamecfg);

        // Read config file if present.
        if (fs::exists(configPath))
        {
            std::ifstream stream;
            stream.exceptions(std::ifstream::failbit);
            stream.open(configPath, std::ios::in | std::ios::binary);
            if (stream.is_open())
            {
                uint32_t magicNumber{};
                stream.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));
                if (magicNumber == kLegacyConfigMagicNumber)
                {
                    stream.read(reinterpret_cast<char*>(&getLegacy()), sizeof(LocoConfig));
                    return getLegacy();
                }
            }
        }

        // A valid config file could not be read. Use defaults.
        setDefaultsLegacyConfig();
        _50AEAD = 1;
        return getLegacy();
    }

    // 0x00441BB8
    static void writeLegacyConfig()
    {
        std::ofstream stream;
        stream.exceptions(std::ifstream::failbit);
        stream.open(Environment::getPathNoWarning(Environment::PathId::gamecfg), std::ios::out | std::ios::binary);
        if (stream.is_open())
        {
            uint32_t magicNumber = kLegacyConfigMagicNumber;
            stream.write(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));
            stream.write(reinterpret_cast<char*>(&getLegacy()), sizeof(LocoConfig));
        }
    }

    // 0x00441BB8
    void write()
    {
        getLegacy() = _newConfig.old;
        writeLegacyConfig();
        writeNewConfig();
    }

    static void readShortcutConfig(const YAML::Node& scNode)
    {
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto& shortcuts = _newConfig.shortcuts;
        for (const auto& def : shortcutDefs)
        {
            auto node = scNode[def.configName];
            if (node)
                shortcuts[def.id] = node.as<KeyboardShortcut>();
            else
                shortcuts[def.id] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
        }
    }

    static NewConfig& readNew()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::openlocoYML);

        // No config file? Use defaults.
        if (!fs::exists(configPath))
        {
            readShortcutConfig(YAML::Node());
            return _newConfig;
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
            auto& displayConfig = _newConfig.display;
            displayConfig.mode = displayNode["mode"].as<ScreenMode>(ScreenMode::window);
            displayConfig.index = displayNode["index"].as<int32_t>(0);
            displayConfig.windowResolution = displayNode["window_resolution"].as<Resolution>(Resolution{ 800, 600 });
            displayConfig.fullscreenResolution = displayNode["fullscreen_resolution"].as<Resolution>(Resolution{ 1920, 1080 });
        }

        // Audio settings
        auto& audioNode = config["audio"];
        if (audioNode && audioNode.IsMap())
        {
            auto& audioConfig = _newConfig.audio;
            audioConfig.device = audioNode["device"].as<std::string>("");
            audioConfig.playTitleMusic = audioNode["play_title_music"].as<bool>(true);
            audioConfig.playNewsSounds = audioNode["play_news_sounds"].as<bool>(true);
        }

        // Network settings
        auto& networkNode = config["network"];
        if (networkNode && networkNode.IsMap())
        {
            auto& networkConfig = _newConfig.network;
            networkConfig.enabled = networkNode["enabled"] && networkNode["enabled"].as<bool>(false);
        }

        // General
        _newConfig.locoInstallPath = config["loco_install_path"].as<std::string>();
        _newConfig.lastSavePath = config["last_save_path"].as<std::string>();
        _newConfig.language = config["language"].as<std::string>();

        // Rendering
        _newConfig.scaleFactor = config["scale_factor"].as<float>(1.0f);
        _newConfig.showFPS = config["showFPS"].as<bool>(false);
        _newConfig.uncapFPS = config["uncapFPS"].as<bool>(false);

        // General UI
        _newConfig.allowMultipleInstances = config["allow_multiple_instances"].as<bool>(false);
        _newConfig.cashPopupRendering = config["cashPopupRendering"].as<bool>(true);
        _newConfig.edgeScrolling = config["edgeScrolling"].as<bool>(true);
        _newConfig.edgeScrollingSpeed = config["edgeScrollingSpeed"].as<int32_t>(12);
        _newConfig.zoomToCursor = config["zoom_to_cursor"].as<bool>(true);

        // Autosaves
        _newConfig.autosaveAmount = config["autosave_amount"].as<int32_t>(12);
        _newConfig.autosaveFrequency = config["autosave_frequency"].as<int32_t>(1);

        // Cheats
        _newConfig.breakdownsDisabled = config["breakdowns_disabled"].as<bool>(false);
        _newConfig.buildLockedVehicles = config["buildLockedVehicles"].as<bool>(false);
        _newConfig.cheatsMenuEnabled = config["cheats_menu_enabled"].as<bool>(false);
        _newConfig.companyAIDisabled = config["companyAIDisabled"].as<bool>(false);
        _newConfig.disableVehicleLoadPenaltyCheat = config["disableVehicleLoadPenaltyCheat"].as<bool>(false);
        _newConfig.displayLockedVehicles = config["displayLockedVehicles"].as<bool>(false);
        _newConfig.invertRightMouseViewPan = config["invertRightMouseViewPan"].as<bool>(false);
        _newConfig.townGrowthDisabled = config["townGrowthDisabled"].as<bool>(false);
        _newConfig.trainsReverseAtSignals = config["trainsReverseAtSignals"].as<bool>(false);

        // Shortcuts
        auto& scNode = config["shortcuts"];
        // Protect from empty shortcuts
        readShortcutConfig(scNode ? scNode : YAML::Node{});
        return _newConfig;
    }

    NewConfig& read()
    {
        auto& config = readNew();
        config.old = readLegacy();
        return config;
    }

    static void writeNewConfig()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::openlocoYML);
        auto dir = configPath.parent_path();
        Environment::autoCreateDirectory(dir);

        auto& node = _configYaml;

        // Display
        const auto& displayConfig = _newConfig.display;
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
        node["display"] = displayNode;

        // Audio
        const auto& audioConfig = _newConfig.audio;
        auto audioNode = node["audio"];
        audioNode["device"] = audioConfig.device;
        if (audioConfig.device.empty())
        {
            audioNode.remove("device");
        }
        audioNode["play_title_music"] = audioConfig.playTitleMusic;
        node["audio"] = audioNode;

        // Network
        const auto& networkConfig = _newConfig.network;
        auto networkNode = node["network"];
        networkNode["enabled"] = networkConfig.enabled;
        node["network"] = networkNode;

        // General
        node["loco_install_path"] = _newConfig.locoInstallPath;
        node["last_save_path"] = _newConfig.lastSavePath;
        node["language"] = _newConfig.language;

        // Rendering
        node["scale_factor"] = _newConfig.scaleFactor;
        node["showFPS"] = _newConfig.showFPS;
        node["uncapFPS"] = _newConfig.uncapFPS;

        // General UI
        node["allow_multiple_instances"] = _newConfig.allowMultipleInstances;
        node["cashPopupRendering"] = _newConfig.cashPopupRendering;
        node["edgeScrolling"] = _newConfig.edgeScrolling;
        node["edgeScrollingSpeed"] = _newConfig.edgeScrollingSpeed;
        node["zoom_to_cursor"] = _newConfig.zoomToCursor;

        // Autosaves
        node["autosave_amount"] = _newConfig.autosaveAmount;
        node["autosave_frequency"] = _newConfig.autosaveFrequency;

        // Cheats
        node["breakdowns_disabled"] = _newConfig.breakdownsDisabled;
        node["buildLockedVehicles"] = _newConfig.buildLockedVehicles;
        node["cheats_menu_enabled"] = _newConfig.cheatsMenuEnabled;
        node["companyAIDisabled"] = _newConfig.companyAIDisabled;
        node["disableVehicleLoadPenaltyCheat"] = _newConfig.disableVehicleLoadPenaltyCheat;
        node["displayLockedVehicles"] = _newConfig.displayLockedVehicles;
        node["invertRightMouseViewPan"] = _newConfig.invertRightMouseViewPan;
        node["townGrowthDisabled"] = _newConfig.townGrowthDisabled;
        node["trainsReverseAtSignals"] = _newConfig.trainsReverseAtSignals;

        // Shortcuts
        const auto& shortcuts = _newConfig.shortcuts;
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto scNode = node["shortcuts"];
        for (const auto& def : shortcutDefs)
        {
            auto it = shortcuts.find(def.id);
            if (it != std::end(shortcuts))
                scNode[def.configName] = it->second;
            else
                scNode[def.configName] = "";
        }
        node["shortcuts"] = scNode;

        std::ofstream stream(configPath);
        if (stream.is_open())
        {
            stream << node << std::endl;
        }
    }

    // 0x004BE3F3
    void resetShortcuts()
    {
        const auto& shortcutDefs = Input::ShortcutManager::getList();

        auto& shortcuts = _newConfig.shortcuts;
        shortcuts.clear();

        for (const auto& def : shortcutDefs)
        {
            shortcuts[def.id] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
        }

        write();
    }

    void registerHooks()
    {
        registerHook(
            0x00441BB8,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto& newConfig = get();
                // Copy the old config into the new config as callers will be modifying the old memory
                newConfig.old = getLegacy();
                write();
                regs = backup;
                return 0;
            });
    }
}
