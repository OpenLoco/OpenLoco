#include "Config.h"
#include "ConfigConvert.hpp"
#include "Core/FileSystem.hpp"
#include "Environment.h"
#include "Input/ShortcutManager.h"
#include "Interop/Interop.hpp"
#include "Utility/Yaml.hpp"
#include <fstream>

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

    constexpr uint8_t _defaultMaxVehicleSounds[3] = { 4, 8, 16 };
    constexpr uint8_t _defaultMaxSoundInstances[3] = { 6, 8, 10 };
    constexpr ObjectHeader _defaultPreferredCurrency = { 0x00000082u, { 'C', 'U', 'R', 'R', 'D', 'O', 'L', 'L' }, 0u };
    constexpr uint32_t _legacyConfigMagicNumber = 0x62272;

    static void writeNewConfig();

    static void setDefaultsLegacyConfig()
    {
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
        getLegacy().maxVehicleSounds = _defaultMaxVehicleSounds[getLegacy().soundQuality];
        getLegacy().maxSoundInstances = _defaultMaxSoundInstances[getLegacy().soundQuality];
        getLegacy().preferredCurrency = _defaultPreferredCurrency;
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
                if (magicNumber == _legacyConfigMagicNumber)
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
            uint32_t magicNumber = _legacyConfigMagicNumber;
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

    static void readShortcutConfig(YAML::Node scNode)
    {
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto& shortcuts = _newConfig.shortcuts;
        for (size_t i = 0; i < std::size(shortcuts); i++)
        {
            auto& def = shortcutDefs[i];
            if (scNode && scNode.IsMap() && scNode[def.configName])
                shortcuts[i] = scNode[def.configName].as<KeyboardShortcut>();
            else
                shortcuts[i] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
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

        auto& displayNode = config["display"];
        if (displayNode && displayNode.IsMap())
        {
            auto& displayConfig = _newConfig.display;
            displayConfig.mode = displayNode["mode"].as<ScreenMode>(ScreenMode::window);
            displayConfig.index = displayNode["index"].as<int32_t>(0);
            displayConfig.windowResolution = displayNode["window_resolution"].as<Resolution>();
            displayConfig.fullscreenResolution = displayNode["fullscreen_resolution"].as<Resolution>();
        }

        auto& audioNode = config["audio"];
        if (audioNode && audioNode.IsMap())
        {
            auto& audioConfig = _newConfig.audio;
            audioConfig.device = audioNode["device"].as<std::string>("");
            if (audioNode["play_title_music"])
                audioConfig.playTitleMusic = audioNode["play_title_music"].as<bool>();
        }

        auto& networkNode = config["network"];
        if (networkNode && networkNode.IsMap())
        {
            auto& networkConfig = _newConfig.network;
            networkConfig.enabled = networkNode["enabled"] && networkNode["enabled"].as<bool>();
        }

        if (config["allow_multiple_instances"])
            _newConfig.allowMultipleInstances = config["allow_multiple_instances"].as<bool>();
        if (config["loco_install_path"])
            _newConfig.locoInstallPath = config["loco_install_path"].as<std::string>();
        if (config["last_save_path"])
            _newConfig.lastSavePath = config["last_save_path"].as<std::string>();
        if (config["language"])
            _newConfig.language = config["language"].as<std::string>();
        if (config["breakdowns_disabled"])
            _newConfig.breakdownsDisabled = config["breakdowns_disabled"].as<bool>();
        if (config["trainsReverseAtSignals"])
            _newConfig.trainsReverseAtSignals = config["trainsReverseAtSignals"].as<bool>();
        if (config["cheats_menu_enabled"])
            _newConfig.cheatsMenuEnabled = config["cheats_menu_enabled"].as<bool>();
        if (config["companyAIDisabled"])
            _newConfig.companyAIDisabled = config["companyAIDisabled"].as<bool>();
        if (config["scale_factor"])
            _newConfig.scaleFactor = config["scale_factor"].as<float>();
        if (config["zoom_to_cursor"])
            _newConfig.zoomToCursor = config["zoom_to_cursor"].as<bool>();
        if (config["autosave_frequency"])
            _newConfig.autosaveFrequency = config["autosave_frequency"].as<int32_t>();
        if (config["autosave_amount"])
            _newConfig.autosaveAmount = config["autosave_amount"].as<int32_t>();
        if (config["showFPS"])
            _newConfig.showFPS = config["showFPS"].as<bool>();
        if (config["uncapFPS"])
            _newConfig.uncapFPS = config["uncapFPS"].as<bool>();
        if (config["displayLockedVehicles"])
            _newConfig.displayLockedVehicles = config["displayLockedVehicles"].as<bool>();
        if (config["buildLockedVehicles"])
            _newConfig.buildLockedVehicles = config["buildLockedVehicles"].as<bool>();
        if (config["invertRightMouseViewPan"])
            _newConfig.invertRightMouseViewPan = config["invertRightMouseViewPan"].as<bool>();
        if (config["cashPopupRendering"])
            _newConfig.cashPopupRendering = config["cashPopupRendering"].as<bool>();

        auto& scNode = config["shortcuts"];
        readShortcutConfig(scNode);

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

        node["allow_multiple_instances"] = _newConfig.allowMultipleInstances;
        node["loco_install_path"] = _newConfig.locoInstallPath;
        node["last_save_path"] = _newConfig.lastSavePath;
        node["language"] = _newConfig.language;
        node["breakdowns_disabled"] = _newConfig.breakdownsDisabled;
        node["trainsReverseAtSignals"] = _newConfig.trainsReverseAtSignals;
        node["cheats_menu_enabled"] = _newConfig.cheatsMenuEnabled;
        node["companyAIDisabled"] = _newConfig.companyAIDisabled;
        node["scale_factor"] = _newConfig.scaleFactor;
        node["zoom_to_cursor"] = _newConfig.zoomToCursor;
        node["autosave_frequency"] = _newConfig.autosaveFrequency;
        node["autosave_amount"] = _newConfig.autosaveAmount;
        node["showFPS"] = _newConfig.showFPS;
        node["uncapFPS"] = _newConfig.uncapFPS;
        node["displayLockedVehicles"] = _newConfig.displayLockedVehicles;
        node["buildLockedVehicles"] = _newConfig.buildLockedVehicles;
        node["invertRightMouseViewPan"] = _newConfig.invertRightMouseViewPan;
        node["cashPopupRendering"] = _newConfig.cashPopupRendering;

        // Shortcuts
        const auto& shortcuts = _newConfig.shortcuts;
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto scNode = node["shortcuts"];
        for (size_t i = 0; i < std::size(shortcuts); i++)
        {
            auto& def = shortcutDefs[i];
            scNode[def.configName] = shortcuts[i];
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
        for (size_t i = 0; i < std::size(shortcuts); i++)
        {
            auto& def = shortcutDefs[i];
            shortcuts[i] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
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
