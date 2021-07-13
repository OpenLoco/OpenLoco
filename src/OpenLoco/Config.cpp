#include <fstream>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include "Config.h"
#include "ConfigConvert.hpp"
#include "Core/FileSystem.hpp"
#include "Environment.h"
#include "Interop/Interop.hpp"
#include "Utility/Yaml.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Config
{
    static loco_global<LocoConfig, 0x0050AEB4> _config;
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

    // 0x00441A6C
    LocoConfig& read()
    {
        call(0x00441A6C);
        return _config;
    }

    // 0x00441BB8
    void write()
    {
        call(0x00441BB8);
        writeNewConfig();
    }

    NewConfig& readNewConfig()
    {
        auto configPath = Environment::getPathNoWarning(Environment::path_id::openloco_yml);

        if (!fs::exists(configPath))
            return _new_config;

        // WARNING: on Windows, YAML::LoadFile only supports ANSI paths
        _config_yaml = YAML::LoadFile(configPath.string());

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

        std::ofstream stream(configPath);
        if (stream.is_open())
        {
            stream << node << std::endl;
        }
    }
}
