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
    static loco_global<LocoConfig, 0x0050AEB4> _config;
    static loco_global<uint8_t, 0x0050AEAD> _50AEAD;
    static loco_global<uint32_t, 0x0113E21C> _113E21C;
    static NewConfig _new_config;
    static YAML::Node _config_yaml;

    LocoConfig& get()
    {
        return _config;
    }

    NewConfig& getNew()
    {
        return _new_config;
    }

    constexpr uint8_t _defaultMaxVehicleSounds[3] = { 4, 8, 16 };
    constexpr uint8_t _defaultMaxSoundInstances[3] = { 6, 8, 10 };
    constexpr ObjectHeader _defaultPreferredCurrency = { 0x00000082u, { 'C', 'U', 'R', 'R', 'D', 'O', 'L', 'L' }, 0u };
    constexpr uint32_t _legacyConfigMagicNumber = 0x62272;

    static void setDefaultsLegacyConfig()
    {
        if (_113E21C > 0x4000000)
        {
            _config->soundQuality = 1;
            _config->vehiclesMinScale = 1;
        }
        else if (_113E21C > 0x8000000)
        {
            _config->soundQuality = 2;
            _config->vehiclesMinScale = 2;
        }
        else
        {
            _config->soundQuality = 0;
            _config->vehiclesMinScale = 1;
        }
        _config->maxVehicleSounds = _defaultMaxVehicleSounds[_config->soundQuality];
        _config->maxSoundInstances = _defaultMaxSoundInstances[_config->soundQuality];
        _config->preferredCurrency = _defaultPreferredCurrency;
    }

    // 0x00441A6C
    LocoConfig& read()
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
                    stream.read(reinterpret_cast<char*>(&*_config), sizeof(LocoConfig));
                    return _config;
                }
            }
        }

        // A valid config file could not be read. Use defaults.
        setDefaultsLegacyConfig();
        _50AEAD = 1;
        return _config;
    }

    // 0x00441BB8
    static void writeLocoConfig()
    {
        std::ofstream stream;
        stream.exceptions(std::ifstream::failbit);
        stream.open(Environment::getPathNoWarning(Environment::PathId::gamecfg), std::ios::out | std::ios::binary);
        if (stream.is_open())
        {
            uint32_t magicNumber = _legacyConfigMagicNumber;
            stream.write(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));
            stream.write(reinterpret_cast<char*>(&*_config), sizeof(LocoConfig));
        }
    }

    // 0x00441BB8
    void write()
    {
        writeLocoConfig();
        writeNewConfig();
    }

    static void readShortcutConfig(YAML::Node scNode)
    {
        const auto& shortcutDefs = Input::ShortcutManager::getList();
        auto& shortcuts = _new_config.shortcuts;
        for (size_t i = 0; i < std::size(shortcuts); i++)
        {
            auto& def = shortcutDefs[i];
            if (scNode && scNode.IsMap() && scNode[def.configName])
                shortcuts[i] = scNode[def.configName].as<KeyboardShortcut>();
            else
                shortcuts[i] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
        }
    }

    NewConfig& readNewConfig()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::openlocoYML);

        // No config file? Use defaults.
        if (!fs::exists(configPath))
        {
            readShortcutConfig(YAML::Node());
            return _new_config;
        }

        // On Windows, YAML::LoadFile only supports ANSI paths, so we pass an ifstream instead.
        std::ifstream stream;
        stream.exceptions(std::ifstream::failbit);
        stream.open(configPath, std::ios::in | std::ios::binary);
        _config_yaml = YAML::Load(stream);

        const auto& config = _config_yaml;

        auto& displayNode = config["display"];
        if (displayNode && displayNode.IsMap())
        {
            auto& displayConfig = _new_config.display;
            displayConfig.mode = displayNode["mode"].as<ScreenMode>(ScreenMode::window);
            displayConfig.index = displayNode["index"].as<int32_t>(0);
            displayConfig.windowResolution = displayNode["window_resolution"].as<Resolution>();
            displayConfig.fullscreenResolution = displayNode["fullscreen_resolution"].as<Resolution>();
        }

        auto& audioNode = config["audio"];
        if (audioNode && audioNode.IsMap())
        {
            auto& audioConfig = _new_config.audio;
            audioConfig.device = audioNode["device"].as<std::string>("");
            if (audioNode["play_title_music"])
                audioConfig.playTitleMusic = audioNode["play_title_music"].as<bool>();
        }

        if (config["loco_install_path"])
            _new_config.locoInstallPath = config["loco_install_path"].as<std::string>();
        if (config["last_save_path"])
            _new_config.lastSavePath = config["last_save_path"].as<std::string>();
        if (config["language"])
            _new_config.language = config["language"].as<std::string>();
        if (config["breakdowns_disabled"])
            _new_config.breakdownsDisabled = config["breakdowns_disabled"].as<bool>();
        if (config["trainsReverseAtSignals"])
            _new_config.trainsReverseAtSignals = config["trainsReverseAtSignals"].as<bool>();
        if (config["cheats_menu_enabled"])
            _new_config.cheatsMenuEnabled = config["cheats_menu_enabled"].as<bool>();
        if (config["companyAIDisabled"])
            _new_config.companyAIDisabled = config["companyAIDisabled"].as<bool>();
        if (config["scale_factor"])
            _new_config.scaleFactor = config["scale_factor"].as<float>();
        if (config["zoom_to_cursor"])
            _new_config.zoomToCursor = config["zoom_to_cursor"].as<bool>();
        if (config["autosave_frequency"])
            _new_config.autosaveFrequency = config["autosave_frequency"].as<int32_t>();
        if (config["autosave_amount"])
            _new_config.autosaveAmount = config["autosave_amount"].as<int32_t>();
        if (config["showFPS"])
            _new_config.showFPS = config["showFPS"].as<bool>();
        if (config["uncapFPS"])
            _new_config.uncapFPS = config["uncapFPS"].as<bool>();
        if (config["displayLockedVehicles"])
            _new_config.displayLockedVehicles = config["displayLockedVehicles"].as<bool>();
        if (config["buildLockedVehicles"])
            _new_config.buildLockedVehicles = config["buildLockedVehicles"].as<bool>();

        auto& scNode = config["shortcuts"];
        readShortcutConfig(scNode);

        return _new_config;
    }

    void writeNewConfig()
    {
        auto configPath = Environment::getPathNoWarning(Environment::PathId::openlocoYML);
        auto dir = configPath.parent_path();
        Environment::autoCreateDirectory(dir);

        auto& node = _config_yaml;

        // Display
        const auto& displayConfig = _new_config.display;
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
        const auto& audioConfig = _new_config.audio;
        auto audioNode = node["audio"];
        audioNode["device"] = audioConfig.device;
        if (audioConfig.device.empty())
        {
            audioNode.remove("device");
        }
        audioNode["play_title_music"] = audioConfig.playTitleMusic;
        node["audio"] = audioNode;

        node["loco_install_path"] = _new_config.locoInstallPath;
        node["last_save_path"] = _new_config.lastSavePath;
        node["language"] = _new_config.language;
        node["breakdowns_disabled"] = _new_config.breakdownsDisabled;
        node["trainsReverseAtSignals"] = _new_config.trainsReverseAtSignals;
        node["cheats_menu_enabled"] = _new_config.cheatsMenuEnabled;
        node["companyAIDisabled"] = _new_config.companyAIDisabled;
        node["scale_factor"] = _new_config.scaleFactor;
        node["zoom_to_cursor"] = _new_config.zoomToCursor;
        node["autosave_frequency"] = _new_config.autosaveFrequency;
        node["autosave_amount"] = _new_config.autosaveAmount;
        node["showFPS"] = _new_config.showFPS;
        node["uncapFPS"] = _new_config.uncapFPS;
        node["displayLockedVehicles"] = _new_config.displayLockedVehicles;
        node["buildLockedVehicles"] = _new_config.buildLockedVehicles;

        // Shortcuts
        const auto& shortcuts = _new_config.shortcuts;
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
        auto& shortcuts = _new_config.shortcuts;
        for (size_t i = 0; i < std::size(shortcuts); i++)
        {
            auto& def = shortcutDefs[i];
            shortcuts[i] = YAML::Node(def.defaultBinding).as<KeyboardShortcut>();
        }

        writeNewConfig();
    }
}
