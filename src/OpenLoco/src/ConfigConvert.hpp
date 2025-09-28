#pragma once

#include "Config.h"
#include "Input.h"
#include "Objects/Object.h"
#include <SDL2/SDL.h>
#include <yaml-cpp/yaml.h>

#define enum_def(x, y) \
    {                  \
        x::y, #y       \
    }

namespace YAML
{
    using namespace OpenLoco::Config;
    using namespace OpenLoco::Input;

    template<typename T>
    using convert_pair_vector = std::vector<std::pair<T, const char*>>;

    template<typename T>
    struct convert_enum_base
    {
        static Node encode(const T& rhs)
        {
            for (const auto& e : convert<T>::getEntries())
            {
                if (rhs == e.first)
                {
                    return Node(e.second);
                }
            }
            return Node();
        }

        static bool decode(const Node& node, T& rhs)
        {
            if (node.IsScalar())
            {
                auto sz = node.Scalar();
                for (const auto& e : convert<T>::getEntries())
                {
                    if (e.second == sz)
                    {
                        rhs = e.first;
                        return true;
                    }
                }
            }
            return false;
        }
    };

    // Keyboard shortcuts
    template<>
    struct convert<KeyboardShortcut>
    {
        static constexpr char kDelimiter = '+';

        static Node encode(const KeyboardShortcut& rhs)
        {
            std::string keyName = "";
            if (rhs.keyCode == 0xFFFFFFFF)
            {
                return Node(keyName);
            }

            if ((rhs.modifiers & KeyModifier::shift) != KeyModifier::none)
            {
                keyName += SDL_GetKeyName(SDLK_LSHIFT);
                keyName += kDelimiter;
            }
            if ((rhs.modifiers & KeyModifier::control) != KeyModifier::none)
            {
                keyName += SDL_GetKeyName(SDLK_LCTRL);
                keyName += kDelimiter;
            }
            if ((rhs.modifiers & KeyModifier::unknown) != KeyModifier::none)
            {
                keyName += SDL_GetKeyName(SDLK_LGUI);
                keyName += kDelimiter;
            }

            keyName += SDL_GetKeyName(rhs.keyCode);
            return Node(keyName);
        }

        static bool decode(const Node& node, KeyboardShortcut& rhs)
        {
            auto s = node.as<std::string>();
            if (s.empty())
            {
                rhs.keyCode = 0xFFFFFFFF;
                rhs.modifiers = KeyModifier::invalid;
                return true;
            }

            rhs.modifiers = KeyModifier::none;
            std::size_t current = 0;
            std::size_t pos = s.find_first_of(kDelimiter, 0);
            std::string token = s;

            while (pos != std::string::npos)
            {
                token = s.substr(current, pos);
                current = pos + 1;
                pos = s.find_first_of(kDelimiter, current);

                SDL_Keycode keyCode = SDL_GetKeyFromName(token.c_str());

                // Check against known modifiers
                if (keyCode == SDLK_LSHIFT || keyCode == SDLK_RSHIFT)
                {
                    rhs.modifiers |= KeyModifier::shift;
                }
                else if (keyCode == SDLK_LCTRL || keyCode == SDLK_RCTRL)
                {
                    rhs.modifiers |= KeyModifier::control;
                }
                else if (keyCode == SDLK_LGUI || keyCode == SDLK_RGUI)
                {
                    rhs.modifiers |= KeyModifier::unknown;
                }
            }

            token = s.substr(current);
            SDL_Keycode keyCode = SDL_GetKeyFromName(token.c_str());
            rhs.keyCode = keyCode;

            return true;
        }
    };

    // OpenLoco::ObjectHeader
    template<>
    struct convert<OpenLoco::ObjectHeader>
    {
        static Node encode(const OpenLoco::ObjectHeader& rhs)
        {
            Node node;
            node["flags"] = rhs.flags;
            node["name"] = std::string(rhs.name, 8);
            node["checksum"] = rhs.checksum;
            return node;
        }

        static bool decode(const Node& node, OpenLoco::ObjectHeader& rhs)
        {
            if (node.IsMap())
            {
                rhs.flags = node["flags"].as<uint32_t>(OpenLoco::kEmptyObjectHeader.flags);
                rhs.checksum = node["checksum"].as<uint32_t>(OpenLoco::kEmptyObjectHeader.checksum);

                auto name = node["name"].as<std::string>(OpenLoco::kEmptyObjectHeader.name);
                memcpy(rhs.name, name.c_str(), 8);
                return true;
            }
            return false;
        }
    };

    // Playlist
    template<>
    struct convert<Playlist>
    {
        static Node encode(const Playlist& rhs)
        {
            Node node;
            node["chuggin_along"] = rhs.chuggin_along;
            node["long_dusty_road"] = rhs.long_dusty_road;
            node["flying_high"] = rhs.flying_high;
            node["gettin_on_the_gas"] = rhs.gettin_on_the_gas;
            node["jumpin_the_rails"] = rhs.jumpin_the_rails;
            node["smooth_running"] = rhs.smooth_running;
            node["traffic_jam"] = rhs.traffic_jam;
            node["never_stop_til_you_get_there"] = rhs.never_stop_til_you_get_there;
            node["soaring_away"] = rhs.soaring_away;
            node["techno_torture"] = rhs.techno_torture;
            node["everlasting_high_rise"] = rhs.everlasting_high_rise;
            node["solace"] = rhs.solace;
            node["chrysanthemum"] = rhs.chrysanthemum;
            node["eugenia"] = rhs.eugenia;
            node["the_ragtime_dance"] = rhs.the_ragtime_dance;
            node["easy_winners"] = rhs.easy_winners;
            node["setting_off"] = rhs.setting_off;
            node["a_travellers_serenade"] = rhs.a_travellers_serenade;
            node["latino_trip"] = rhs.latino_trip;
            node["a_good_head_of_steam"] = rhs.a_good_head_of_steam;
            node["hop_to_the_bop"] = rhs.hop_to_the_bop;
            node["the_city_lights"] = rhs.the_city_lights;
            node["steamin_down_town"] = rhs.steamin_down_town;
            node["bright_expectations"] = rhs.bright_expectations;
            node["mo_station"] = rhs.mo_station;
            node["far_out"] = rhs.far_out;
            node["running_on_time"] = rhs.running_on_time;
            node["get_me_to_gladstone_bay"] = rhs.get_me_to_gladstone_bay;
            node["sandy_track_blues"] = rhs.sandy_track_blues;
            return node;
        }

        static bool decode(const Node& node, Playlist& rhs)
        {
            const bool enableAll = !node.IsMap();
            rhs.chuggin_along = enableAll || node["chuggin_along"].as<bool>(true);
            rhs.long_dusty_road = enableAll || node["long_dusty_road"].as<bool>(true);
            rhs.flying_high = enableAll || node["flying_high"].as<bool>(true);
            rhs.gettin_on_the_gas = enableAll || node["gettin_on_the_gas"].as<bool>(true);
            rhs.jumpin_the_rails = enableAll || node["jumpin_the_rails"].as<bool>(true);
            rhs.smooth_running = enableAll || node["smooth_running"].as<bool>(true);
            rhs.traffic_jam = enableAll || node["traffic_jam"].as<bool>(true);
            rhs.never_stop_til_you_get_there = enableAll || node["never_stop_til_you_get_there"].as<bool>(true);
            rhs.soaring_away = enableAll || node["soaring_away"].as<bool>(true);
            rhs.techno_torture = enableAll || node["techno_torture"].as<bool>(true);
            rhs.everlasting_high_rise = enableAll || node["everlasting_high_rise"].as<bool>(true);
            rhs.solace = enableAll || node["solace"].as<bool>(true);
            rhs.chrysanthemum = enableAll || node["chrysanthemum"].as<bool>(true);
            rhs.eugenia = enableAll || node["eugenia"].as<bool>(true);
            rhs.the_ragtime_dance = enableAll || node["the_ragtime_dance"].as<bool>(true);
            rhs.easy_winners = enableAll || node["easy_winners"].as<bool>(true);
            rhs.setting_off = enableAll || node["setting_off"].as<bool>(true);
            rhs.a_travellers_serenade = enableAll || node["a_travellers_serenade"].as<bool>(true);
            rhs.latino_trip = enableAll || node["latino_trip"].as<bool>(true);
            rhs.a_good_head_of_steam = enableAll || node["a_good_head_of_steam"].as<bool>(true);
            rhs.hop_to_the_bop = enableAll || node["hop_to_the_bop"].as<bool>(true);
            rhs.the_city_lights = enableAll || node["the_city_lights"].as<bool>(true);
            rhs.steamin_down_town = enableAll || node["steamin_down_town"].as<bool>(true);
            rhs.bright_expectations = enableAll || node["bright_expectations"].as<bool>(true);
            rhs.mo_station = enableAll || node["mo_station"].as<bool>(true);
            rhs.far_out = enableAll || node["far_out"].as<bool>(true);
            rhs.running_on_time = enableAll || node["running_on_time"].as<bool>(true);
            rhs.get_me_to_gladstone_bay = enableAll || node["get_me_to_gladstone_bay"].as<bool>(true);
            rhs.sandy_track_blues = enableAll || node["sandy_track_blues"].as<bool>(true);
            return true;
        }
    };

    // Resolution
    template<>
    struct convert<Resolution>
    {
        static Node encode(const Resolution& rhs)
        {
            Node node;
            node["width"] = rhs.width;
            node["height"] = rhs.height;
            return node;
        }

        static bool decode(const Node& node, Resolution& rhs)
        {
            if (node.IsMap())
            {
                rhs.width = node["width"].as<int32_t>();
                rhs.height = node["height"].as<int32_t>();
                return true;
            }
            return false;
        }
    };

    // MeasurementFormat
    const convert_pair_vector<MeasurementFormat> measurementFormatEntries = {
        enum_def(MeasurementFormat, imperial),
        enum_def(MeasurementFormat, metric),
    };
    template<>
    struct convert<MeasurementFormat> : convert_enum_base<MeasurementFormat>
    {
        static const convert_pair_vector<MeasurementFormat>& getEntries() { return measurementFormatEntries; }
    };

    // NewsType
    const convert_pair_vector<NewsType> newsTypeEntries = {
        enum_def(NewsType, none),
        enum_def(NewsType, ticker),
        enum_def(NewsType, newsWindow),
    };
    template<>
    struct convert<NewsType> : convert_enum_base<NewsType>
    {
        static const convert_pair_vector<NewsType>& getEntries() { return newsTypeEntries; }
    };

    // ScreenMode
    const convert_pair_vector<ScreenMode> screenModeEntries = {
        enum_def(ScreenMode, window),
        enum_def(ScreenMode, fullscreen),
        enum_def(ScreenMode, fullscreenBorderless),
    };
    template<>
    struct convert<ScreenMode> : convert_enum_base<ScreenMode>
    {
        static const convert_pair_vector<ScreenMode>& getEntries() { return screenModeEntries; }
    };
}

#undef enum_def
