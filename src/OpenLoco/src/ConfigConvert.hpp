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
            node["chugginAlong"] = rhs.chugginAlong;
            node["longDustyRoad"] = rhs.longDustyRoad;
            node["flyingHigh"] = rhs.flyingHigh;
            node["gettinOnTheGas"] = rhs.gettinOnTheGas;
            node["jumpinTheRails"] = rhs.jumpinTheRails;
            node["smoothRunning"] = rhs.smoothRunning;
            node["trafficJam"] = rhs.trafficJam;
            node["neverStopTilYouGetThere"] = rhs.neverStopTilYouGetThere;
            node["soaringAway"] = rhs.soaringAway;
            node["technoTorture"] = rhs.technoTorture;
            node["everlastingHighRise"] = rhs.everlastingHighRise;
            node["solace"] = rhs.solace;
            node["chrysanthemum"] = rhs.chrysanthemum;
            node["eugenia"] = rhs.eugenia;
            node["theRagtimeDance"] = rhs.theRagtimeDance;
            node["easyWinners"] = rhs.easyWinners;
            node["settingOff"] = rhs.settingOff;
            node["aTravellersSerenade"] = rhs.aTravellersSerenade;
            node["latinoTrip"] = rhs.latinoTrip;
            node["aGoodHeadOfSteam"] = rhs.aGoodHeadOfSteam;
            node["hopToTheBop"] = rhs.hopToTheBop;
            node["theCityLights"] = rhs.theCityLights;
            node["steaminDownTown"] = rhs.steaminDownTown;
            node["brightExpectations"] = rhs.brightExpectations;
            node["moStation"] = rhs.moStation;
            node["farOut"] = rhs.farOut;
            node["runningOnTime"] = rhs.runningOnTime;
            node["getMeToGladstoneBay"] = rhs.getMeToGladstoneBay;
            node["sandyTrackBlues"] = rhs.sandyTrackBlues;
            return node;
        }

        static bool decode(const Node& node, Playlist& rhs)
        {
            const bool enableAll = !node.IsMap();
            rhs.chugginAlong = enableAll || node["chugginAlong"].as<bool>(true);
            rhs.longDustyRoad = enableAll || node["longDustyRoad"].as<bool>(true);
            rhs.flyingHigh = enableAll || node["flyingHigh"].as<bool>(true);
            rhs.gettinOnTheGas = enableAll || node["gettinOnTheGas"].as<bool>(true);
            rhs.jumpinTheRails = enableAll || node["jumpinTheRails"].as<bool>(true);
            rhs.smoothRunning = enableAll || node["smoothRunning"].as<bool>(true);
            rhs.trafficJam = enableAll || node["trafficJam"].as<bool>(true);
            rhs.neverStopTilYouGetThere = enableAll || node["neverStopTilYouGetThere"].as<bool>(true);
            rhs.soaringAway = enableAll || node["soaringAway"].as<bool>(true);
            rhs.technoTorture = enableAll || node["technoTorture"].as<bool>(true);
            rhs.everlastingHighRise = enableAll || node["everlastingHighRise"].as<bool>(true);
            rhs.solace = enableAll || node["solace"].as<bool>(true);
            rhs.chrysanthemum = enableAll || node["chrysanthemum"].as<bool>(true);
            rhs.eugenia = enableAll || node["eugenia"].as<bool>(true);
            rhs.theRagtimeDance = enableAll || node["theRagtimeDance"].as<bool>(true);
            rhs.easyWinners = enableAll || node["easyWinners"].as<bool>(true);
            rhs.settingOff = enableAll || node["settingOff"].as<bool>(true);
            rhs.aTravellersSerenade = enableAll || node["aTravellersSerenade"].as<bool>(true);
            rhs.latinoTrip = enableAll || node["latinoTrip"].as<bool>(true);
            rhs.aGoodHeadOfSteam = enableAll || node["aGoodHeadOfSteam"].as<bool>(true);
            rhs.hopToTheBop = enableAll || node["hopToTheBop"].as<bool>(true);
            rhs.theCityLights = enableAll || node["theCityLights"].as<bool>(true);
            rhs.steaminDownTown = enableAll || node["steaminDownTown"].as<bool>(true);
            rhs.brightExpectations = enableAll || node["brightExpectations"].as<bool>(true);
            rhs.moStation = enableAll || node["moStation"].as<bool>(true);
            rhs.farOut = enableAll || node["farOut"].as<bool>(true);
            rhs.runningOnTime = enableAll || node["runningOnTime"].as<bool>(true);
            rhs.getMeToGladstoneBay = enableAll || node["getMeToGladstoneBay"].as<bool>(true);
            rhs.sandyTrackBlues = enableAll || node["sandyTrackBlues"].as<bool>(true);
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

    // MusicPlaylistType
    const convert_pair_vector<MusicPlaylistType> kMusicPlaylistTypes = {
        enum_def(MusicPlaylistType, currentEra),
        enum_def(MusicPlaylistType, all),
        enum_def(MusicPlaylistType, custom),
    };
    template<>
    struct convert<MusicPlaylistType> : convert_enum_base<MusicPlaylistType>
    {
        static const convert_pair_vector<MusicPlaylistType>& getEntries() { return kMusicPlaylistTypes; }
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
