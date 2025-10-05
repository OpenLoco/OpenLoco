#pragma once

#include "Config.h"
#include "Input.h"
#include "Objects/Object.h"
#include <OpenLoco/Engine/Types.hpp>
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

    enum class PlaylistItem : uint8_t
    {
        chugginAlong,
        longDustyRoad,
        flyingHigh,
        gettinOnTheGas,
        jumpinTheRails,
        smoothRunning,
        trafficJam,
        neverStopTilYouGetThere,
        soaringAway,
        technoTorture,
        everlastingHighRise,
        solace,
        chrysanthemum,
        eugenia,
        theRagtimeDance,
        easyWinners,
        settingOff,
        aTravellersSerenade,
        latinoTrip,
        aGoodHeadOfSteam,
        hopToTheBop,
        theCityLights,
        steaminDownTown,
        brightExpectations,
        moStation,
        farOut,
        runningOnTime,
        getMeToGladstoneBay,
        sandyTrackBlues,
    };

    // Playlist
    template<>
    struct convert<Playlist>
    {
        static Node encode(const Playlist& rhs)
        {
            using namespace OpenLoco;
            Node node;
            node["chugginAlong"] = rhs[enumValue(PlaylistItem::chugginAlong)];
            node["longDustyRoad"] = rhs[enumValue(PlaylistItem::longDustyRoad)];
            node["flyingHigh"] = rhs[enumValue(PlaylistItem::flyingHigh)];
            node["gettinOnTheGas"] = rhs[enumValue(PlaylistItem::gettinOnTheGas)];
            node["jumpinTheRails"] = rhs[enumValue(PlaylistItem::jumpinTheRails)];
            node["smoothRunning"] = rhs[enumValue(PlaylistItem::smoothRunning)];
            node["trafficJam"] = rhs[enumValue(PlaylistItem::trafficJam)];
            node["neverStopTilYouGetThere"] = rhs[enumValue(PlaylistItem::neverStopTilYouGetThere)];
            node["soaringAway"] = rhs[enumValue(PlaylistItem::soaringAway)];
            node["technoTorture"] = rhs[enumValue(PlaylistItem::technoTorture)];
            node["everlastingHighRise"] = rhs[enumValue(PlaylistItem::everlastingHighRise)];
            node["solace"] = rhs[enumValue(PlaylistItem::solace)];
            node["chrysanthemum"] = rhs[enumValue(PlaylistItem::chrysanthemum)];
            node["eugenia"] = rhs[enumValue(PlaylistItem::eugenia)];
            node["theRagtimeDance"] = rhs[enumValue(PlaylistItem::theRagtimeDance)];
            node["easyWinners"] = rhs[enumValue(PlaylistItem::easyWinners)];
            node["settingOff"] = rhs[enumValue(PlaylistItem::settingOff)];
            node["aTravellersSerenade"] = rhs[enumValue(PlaylistItem::aTravellersSerenade)];
            node["latinoTrip"] = rhs[enumValue(PlaylistItem::latinoTrip)];
            node["aGoodHeadOfSteam"] = rhs[enumValue(PlaylistItem::aGoodHeadOfSteam)];
            node["hopToTheBop"] = rhs[enumValue(PlaylistItem::hopToTheBop)];
            node["theCityLights"] = rhs[enumValue(PlaylistItem::theCityLights)];
            node["steaminDownTown"] = rhs[enumValue(PlaylistItem::steaminDownTown)];
            node["brightExpectations"] = rhs[enumValue(PlaylistItem::brightExpectations)];
            node["moStation"] = rhs[enumValue(PlaylistItem::moStation)];
            node["farOut"] = rhs[enumValue(PlaylistItem::farOut)];
            node["runningOnTime"] = rhs[enumValue(PlaylistItem::runningOnTime)];
            node["getMeToGladstoneBay"] = rhs[enumValue(PlaylistItem::getMeToGladstoneBay)];
            node["sandyTrackBlues"] = rhs[enumValue(PlaylistItem::sandyTrackBlues)];
            return node;
        }

        static bool decode(const Node& node, Playlist& rhs)
        {
            using namespace OpenLoco;
            const bool enableAll = !node.IsMap();
            rhs[enumValue(PlaylistItem::chugginAlong)] = enableAll || node["chugginAlong"].as<bool>(true);
            rhs[enumValue(PlaylistItem::longDustyRoad)] = enableAll || node["longDustyRoad"].as<bool>(true);
            rhs[enumValue(PlaylistItem::flyingHigh)] = enableAll || node["flyingHigh"].as<bool>(true);
            rhs[enumValue(PlaylistItem::gettinOnTheGas)] = enableAll || node["gettinOnTheGas"].as<bool>(true);
            rhs[enumValue(PlaylistItem::jumpinTheRails)] = enableAll || node["jumpinTheRails"].as<bool>(true);
            rhs[enumValue(PlaylistItem::smoothRunning)] = enableAll || node["smoothRunning"].as<bool>(true);
            rhs[enumValue(PlaylistItem::trafficJam)] = enableAll || node["trafficJam"].as<bool>(true);
            rhs[enumValue(PlaylistItem::neverStopTilYouGetThere)] = enableAll || node["neverStopTilYouGetThere"].as<bool>(true);
            rhs[enumValue(PlaylistItem::soaringAway)] = enableAll || node["soaringAway"].as<bool>(true);
            rhs[enumValue(PlaylistItem::technoTorture)] = enableAll || node["technoTorture"].as<bool>(true);
            rhs[enumValue(PlaylistItem::everlastingHighRise)] = enableAll || node["everlastingHighRise"].as<bool>(true);
            rhs[enumValue(PlaylistItem::solace)] = enableAll || node["solace"].as<bool>(true);
            rhs[enumValue(PlaylistItem::chrysanthemum)] = enableAll || node["chrysanthemum"].as<bool>(true);
            rhs[enumValue(PlaylistItem::eugenia)] = enableAll || node["eugenia"].as<bool>(true);
            rhs[enumValue(PlaylistItem::theRagtimeDance)] = enableAll || node["theRagtimeDance"].as<bool>(true);
            rhs[enumValue(PlaylistItem::easyWinners)] = enableAll || node["easyWinners"].as<bool>(true);
            rhs[enumValue(PlaylistItem::settingOff)] = enableAll || node["settingOff"].as<bool>(true);
            rhs[enumValue(PlaylistItem::aTravellersSerenade)] = enableAll || node["aTravellersSerenade"].as<bool>(true);
            rhs[enumValue(PlaylistItem::latinoTrip)] = enableAll || node["latinoTrip"].as<bool>(true);
            rhs[enumValue(PlaylistItem::aGoodHeadOfSteam)] = enableAll || node["aGoodHeadOfSteam"].as<bool>(true);
            rhs[enumValue(PlaylistItem::hopToTheBop)] = enableAll || node["hopToTheBop"].as<bool>(true);
            rhs[enumValue(PlaylistItem::theCityLights)] = enableAll || node["theCityLights"].as<bool>(true);
            rhs[enumValue(PlaylistItem::steaminDownTown)] = enableAll || node["steaminDownTown"].as<bool>(true);
            rhs[enumValue(PlaylistItem::brightExpectations)] = enableAll || node["brightExpectations"].as<bool>(true);
            rhs[enumValue(PlaylistItem::moStation)] = enableAll || node["moStation"].as<bool>(true);
            rhs[enumValue(PlaylistItem::farOut)] = enableAll || node["farOut"].as<bool>(true);
            rhs[enumValue(PlaylistItem::runningOnTime)] = enableAll || node["runningOnTime"].as<bool>(true);
            rhs[enumValue(PlaylistItem::getMeToGladstoneBay)] = enableAll || node["getMeToGladstoneBay"].as<bool>(true);
            rhs[enumValue(PlaylistItem::sandyTrackBlues)] = enableAll || node["sandyTrackBlues"].as<bool>(true);
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
    const convert_pair_vector<MeasurementFormat> kMeasurementFormatEntries = {
        enum_def(MeasurementFormat, imperial),
        enum_def(MeasurementFormat, metric),
    };
    template<>
    struct convert<MeasurementFormat> : convert_enum_base<MeasurementFormat>
    {
        static const convert_pair_vector<MeasurementFormat>& getEntries() { return kMeasurementFormatEntries; }
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
    const convert_pair_vector<NewsType> kNewsTypeEntries = {
        enum_def(NewsType, none),
        enum_def(NewsType, ticker),
        enum_def(NewsType, newsWindow),
    };
    template<>
    struct convert<NewsType> : convert_enum_base<NewsType>
    {
        static const convert_pair_vector<NewsType>& getEntries() { return kNewsTypeEntries; }
    };

    // ScreenMode
    const convert_pair_vector<ScreenMode> kScreenModeEntries = {
        enum_def(ScreenMode, window),
        enum_def(ScreenMode, fullscreen),
        enum_def(ScreenMode, fullscreenBorderless),
    };
    template<>
    struct convert<ScreenMode> : convert_enum_base<ScreenMode>
    {
        static const convert_pair_vector<ScreenMode>& getEntries() { return kScreenModeEntries; }
    };
}

#undef enum_def
