#include "Config.h"
#include "ConfigConvert.hpp"
#include "Core/FileSystem.hpp"
#include "Environment.h"
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
            _config->sound_quality = 1;
            _config->vehicles_min_scale = 1;
        }
        else if (_113E21C > 0x8000000)
        {
            _config->sound_quality = 2;
            _config->vehicles_min_scale = 2;
        }
        else
        {
            _config->sound_quality = 0;
            _config->vehicles_min_scale = 1;
        }
        _config->max_vehicle_sounds = _defaultMaxVehicleSounds[_config->sound_quality];
        _config->max_sound_instances = _defaultMaxSoundInstances[_config->sound_quality];
        _config->preferred_currency = _defaultPreferredCurrency;
    }

    // 0x00441A6C
    LocoConfig& read()
    {
        std::ifstream stream;
        stream.exceptions(std::ifstream::failbit);
        stream.open(Environment::getPathNoWarning(Environment::path_id::gamecfg), std::ios::in | std::ios::binary);
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

        setDefaultsLegacyConfig();
        _50AEAD = 1;
        return _config;
    }

    // 0x00441BB8
    static void writeLocoConfig()
    {
        std::ofstream stream;
        stream.exceptions(std::ifstream::failbit);
        stream.open(Environment::getPathNoWarning(Environment::path_id::gamecfg), std::ios::out | std::ios::binary);
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

    NewConfig& readNewConfig()
    {
        auto configPath = Environment::getPathNoWarning(Environment::path_id::openloco_yml);

        if (!fs::exists(configPath))
            return _new_config;

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
            displayConfig.window_resolution = displayNode["window_resolution"].as<Resolution>();
            displayConfig.fullscreen_resolution = displayNode["fullscreen_resolution"].as<Resolution>();
        }

        auto& audioNode = config["audio"];
        if (audioNode && audioNode.IsMap())
        {
            auto& audioConfig = _new_config.audio;
            audioConfig.device = audioNode["device"].as<std::string>("");
            if (audioNode["play_title_music"])
                audioConfig.play_title_music = audioNode["play_title_music"].as<bool>();
        }

        if (config["loco_install_path"])
            _new_config.loco_install_path = config["loco_install_path"].as<std::string>();
        if (config["last_save_path"])
            _new_config.last_save_path = config["last_save_path"].as<std::string>();
        if (config["language"])
            _new_config.language = config["language"].as<std::string>();
        if (config["breakdowns_disabled"])
            _new_config.breakdowns_disabled = config["breakdowns_disabled"].as<bool>();
        if (config["cheats_menu_enabled"])
            _new_config.cheats_menu_enabled = config["cheats_menu_enabled"].as<bool>();
        if (config["companyAIDisabled"])
            _new_config.companyAIDisabled = config["companyAIDisabled"].as<bool>();
        if (config["scale_factor"])
            _new_config.scale_factor = config["scale_factor"].as<float>();
        if (config["zoom_to_cursor"])
            _new_config.zoom_to_cursor = config["zoom_to_cursor"].as<bool>();
        if (config["autosave_frequency"])
            _new_config.autosave_frequency = config["autosave_frequency"].as<int32_t>();
        if (config["autosave_amount"])
            _new_config.autosave_amount = config["autosave_amount"].as<int32_t>();
        if (config["showFPS"])
            _new_config.showFPS = config["showFPS"].as<bool>();
        if (config["uncapFPS"])
            _new_config.uncapFPS = config["uncapFPS"].as<bool>();

        auto& scNode = config["shortcuts"];
        if (scNode && scNode.IsMap())
        {
            auto& shortcuts = _new_config.shortcuts;
            shortcuts.closeTopmostWindow = scNode["closeTopmostWindow"].as<KeyboardShortcut>();
            shortcuts.closeAllFloatingWindows = scNode["closeAllFloatingWindows"].as<KeyboardShortcut>();
            shortcuts.cancelConstructionMode = scNode["cancelConstructionMode"].as<KeyboardShortcut>();
            shortcuts.pauseUnpauseGame = scNode["pauseUnpauseGame"].as<KeyboardShortcut>();
            shortcuts.zoomViewOut = scNode["zoomViewOut"].as<KeyboardShortcut>();
            shortcuts.zoomViewIn = scNode["zoomViewIn"].as<KeyboardShortcut>();
            shortcuts.rotateView = scNode["rotateView"].as<KeyboardShortcut>();
            shortcuts.rotateConstructionObject = scNode["rotateConstructionObject"].as<KeyboardShortcut>();
            shortcuts.toggleUndergroundView = scNode["toggleUndergroundView"].as<KeyboardShortcut>();
            shortcuts.toggleHideForegroundTracks = scNode["toggleHideForegroundTracks"].as<KeyboardShortcut>();
            shortcuts.toggleHideForegroundScenery = scNode["toggleHideForegroundScenery"].as<KeyboardShortcut>();
            shortcuts.toggleHeightMarksOnLand = scNode["toggleHeightMarksOnLand"].as<KeyboardShortcut>();
            shortcuts.toggleHeightMarksOnTracks = scNode["toggleHeightMarksOnTracks"].as<KeyboardShortcut>();
            shortcuts.toggleDirArrowsOnTracks = scNode["toggleDirArrowsOnTracks"].as<KeyboardShortcut>();
            shortcuts.adjustLand = scNode["adjustLand"].as<KeyboardShortcut>();
            shortcuts.adjustWater = scNode["adjustWater"].as<KeyboardShortcut>();
            shortcuts.plantTrees = scNode["plantTrees"].as<KeyboardShortcut>();
            shortcuts.bulldozeArea = scNode["bulldozeArea"].as<KeyboardShortcut>();
            shortcuts.buildTracks = scNode["buildTracks"].as<KeyboardShortcut>();
            shortcuts.buildRoads = scNode["buildRoads"].as<KeyboardShortcut>();
            shortcuts.buildAirports = scNode["buildAirports"].as<KeyboardShortcut>();
            shortcuts.buildShipPorts = scNode["buildShipPorts"].as<KeyboardShortcut>();
            shortcuts.buildNewVehicles = scNode["buildNewVehicles"].as<KeyboardShortcut>();
            shortcuts.showVehiclesList = scNode["showVehiclesList"].as<KeyboardShortcut>();
            shortcuts.showStationsList = scNode["showStationsList"].as<KeyboardShortcut>();
            shortcuts.showTownsList = scNode["showTownsList"].as<KeyboardShortcut>();
            shortcuts.showIndustriesList = scNode["showIndustriesList"].as<KeyboardShortcut>();
            shortcuts.showMap = scNode["showMap"].as<KeyboardShortcut>();
            shortcuts.showCompaniesList = scNode["showCompaniesList"].as<KeyboardShortcut>();
            shortcuts.showCompanyInformation = scNode["showCompanyInformation"].as<KeyboardShortcut>();
            shortcuts.showFinances = scNode["showFinances"].as<KeyboardShortcut>();
            shortcuts.showAnnouncementsList = scNode["showAnnouncementsList"].as<KeyboardShortcut>();
            shortcuts.makeScreenshot = scNode["makeScreenshot"].as<KeyboardShortcut>();
            shortcuts.toggleLastAnnouncement = scNode["toggleLastAnnouncement"].as<KeyboardShortcut>();
            shortcuts.sendMessage = scNode["sendMessage"].as<KeyboardShortcut>();
        }

        return _new_config;
    }

    void writeNewConfig()
    {
        auto configPath = Environment::getPathNoWarning(Environment::path_id::openloco_yml);
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
        displayNode["window_resolution"] = displayConfig.window_resolution;
        displayNode["fullscreen_resolution"] = displayConfig.fullscreen_resolution;
        node["display"] = displayNode;

        // Audio
        const auto& audioConfig = _new_config.audio;
        auto audioNode = node["audio"];
        audioNode["device"] = audioConfig.device;
        if (audioConfig.device.empty())
        {
            audioNode.remove("device");
        }
        audioNode["play_title_music"] = audioConfig.play_title_music;
        node["audio"] = audioNode;

        node["loco_install_path"] = _new_config.loco_install_path;
        node["last_save_path"] = _new_config.last_save_path;
        node["language"] = _new_config.language;
        node["breakdowns_disabled"] = _new_config.breakdowns_disabled;
        node["cheats_menu_enabled"] = _new_config.cheats_menu_enabled;
        node["companyAIDisabled"] = _new_config.companyAIDisabled;
        node["scale_factor"] = _new_config.scale_factor;
        node["zoom_to_cursor"] = _new_config.zoom_to_cursor;
        node["autosave_frequency"] = _new_config.autosave_frequency;
        node["autosave_amount"] = _new_config.autosave_amount;
        node["showFPS"] = _new_config.showFPS;
        node["uncapFPS"] = _new_config.uncapFPS;

        // Shortcuts
        const auto& shortcuts = _new_config.shortcuts;
        auto scNode = node["shortcuts"];
        scNode["closeTopmostWindow"] = shortcuts.closeTopmostWindow;
        scNode["closeAllFloatingWindows"] = shortcuts.closeAllFloatingWindows;
        scNode["cancelConstructionMode"] = shortcuts.cancelConstructionMode;
        scNode["pauseUnpauseGame"] = shortcuts.pauseUnpauseGame;
        scNode["zoomViewOut"] = shortcuts.zoomViewOut;
        scNode["zoomViewIn"] = shortcuts.zoomViewIn;
        scNode["rotateView"] = shortcuts.rotateView;
        scNode["rotateConstructionObject"] = shortcuts.rotateConstructionObject;
        scNode["toggleUndergroundView"] = shortcuts.toggleUndergroundView;
        scNode["toggleHideForegroundTracks"] = shortcuts.toggleHideForegroundTracks;
        scNode["toggleHideForegroundScenery"] = shortcuts.toggleHideForegroundScenery;
        scNode["toggleHeightMarksOnLand"] = shortcuts.toggleHeightMarksOnLand;
        scNode["toggleHeightMarksOnTracks"] = shortcuts.toggleHeightMarksOnTracks;
        scNode["toggleDirArrowsOnTracks"] = shortcuts.toggleDirArrowsOnTracks;
        scNode["adjustLand"] = shortcuts.adjustLand;
        scNode["adjustWater"] = shortcuts.adjustWater;
        scNode["plantTrees"] = shortcuts.plantTrees;
        scNode["bulldozeArea"] = shortcuts.bulldozeArea;
        scNode["buildTracks"] = shortcuts.buildTracks;
        scNode["buildRoads"] = shortcuts.buildRoads;
        scNode["buildAirports"] = shortcuts.buildAirports;
        scNode["buildShipPorts"] = shortcuts.buildShipPorts;
        scNode["buildNewVehicles"] = shortcuts.buildNewVehicles;
        scNode["showVehiclesList"] = shortcuts.showVehiclesList;
        scNode["showStationsList"] = shortcuts.showStationsList;
        scNode["showTownsList"] = shortcuts.showTownsList;
        scNode["showIndustriesList"] = shortcuts.showIndustriesList;
        scNode["showMap"] = shortcuts.showMap;
        scNode["showCompaniesList"] = shortcuts.showCompaniesList;
        scNode["showCompanyInformation"] = shortcuts.showCompanyInformation;
        scNode["showFinances"] = shortcuts.showFinances;
        scNode["showAnnouncementsList"] = shortcuts.showAnnouncementsList;
        scNode["makeScreenshot"] = shortcuts.makeScreenshot;
        scNode["toggleLastAnnouncement"] = shortcuts.toggleLastAnnouncement;
        scNode["sendMessage"] = shortcuts.sendMessage;
        node["shortcuts"] = scNode;

        std::ofstream stream(configPath);
        if (stream.is_open())
        {
            stream << node << std::endl;
        }
    }
}
