#include "LanguageFiles.h"
#include "../Config.h"
#include "../Console.h"
#include "../Environment.h"
#include "../Interop/Interop.hpp"
#include "../Platform/Platform.h"
#include "../Utility/Yaml.hpp"
#include "Conversion.h"
#include "StringIds.h"
#include "StringManager.h"
#include "Unicode.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace OpenLoco::Interop;

namespace OpenLoco::Localisation
{
    static loco_global<char* [0xFFFF], 0x005183FC> _strings;
    static std::vector<std::unique_ptr<char[]>> _strings_owner;

    static std::map<std::string, uint8_t, std::less<>> basicCommands = {
        { "INT16_1DP", ControlCodes::int16_decimals },
        { "INT32_1DP", ControlCodes::int32_decimals },
        { "INT16", ControlCodes::int16_grouped },
        { "UINT16", ControlCodes::uint16_ungrouped },
        { "SMALLFONT", ControlCodes::font_regular },
        { "BIGFONT", ControlCodes::font_large },
        { "TINYFONT", ControlCodes::font_small },
        { "NEWLINE_SMALLER", ControlCodes::newline_smaller },
        { "OUTLINE", OpenLoco::ControlCodes::outline },
        { "VELOCITY", ControlCodes::velocity },
        { "CURRENCY32", ControlCodes::currency32 },
        { "HEIGHT", ControlCodes::height },
        { "CURRENCY48", ControlCodes::currency48 },
        { "STRING", ControlCodes::string_ptr },
        { "POP16", ControlCodes::pop16 },
        { "POWER", ControlCodes::power },
    };

    static std::map<std::string, uint8_t, std::less<>> textColourNames = {
        { "BLACK", ControlCodes::colour_black },
        { "WINDOW_1", ControlCodes::window_colour_1 },
        { "WINDOW_2", ControlCodes::window_colour_2 },
        { "WINDOW_3", ControlCodes::window_colour_3 },
        { "WINDOW_4", ControlCodes::window_colour_4 },
        { "WHITE", ControlCodes::colour_white },
        { "YELLOW", ControlCodes::colour_yellow },
        { "TOPAZ", ControlCodes::colour_topaz },
        { "RED", ControlCodes::colour_red },
        { "GREEN", ControlCodes::colour_green },
    };

    static std::unique_ptr<char[]> readString(const char* value, size_t size)
    {
        // Take terminating NULL character in account
        auto str = std::make_unique<char[]>(size + 1);
        char* out = str.get();

        utf8_t* ptr = (utf8_t*)value;
        while (true)
        {
            utf32_t codepoint = readCodePoint(&ptr);
            if (codepoint == UnicodeChar::superscript_minus || codepoint == UnicodeChar::variation_selector)
                continue;

            char readChar = convertUnicodeToLoco(codepoint);
            if (readChar == '{')
            {
                std::vector<std::string_view> commands = {};
                char* start = nullptr;
                while (true)
                {
                    char* pos = (char*)ptr;
                    readChar = readCodePoint(&ptr);

                    if (readChar == ' ')
                    {
                        if (start != nullptr)
                        {
                            commands.push_back(std::string_view(start, pos - start));
                        }
                        start = nullptr;
                        continue;
                    }

                    if (readChar == '}')
                    {
                        if (start != nullptr)
                        {
                            commands.push_back(std::string_view(start, pos - start));
                        }
                        break;
                    }

                    if (start == nullptr)
                    {
                        start = pos;
                    }
                }

                auto search = basicCommands.find(commands[0]);
                if (search != basicCommands.end())
                {
                    *out = search->second;
                    out++;
                }
                else if (commands[0] == "STRINGID")
                {
                    if (commands.size() == 1)
                    {
                        *out = (char)ControlCodes::stringid_args;
                        out++;
                    }
                    else
                    {
                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "UINT16")
                {
                    *out = (char)ControlCodes::uint16_ungrouped;
                    out++;
                }
                else if (commands[0] == "SPRITE")
                {
                    if (commands.size() == 1)
                    {
                        *out++ = (char)ControlCodes::inline_sprite_args;
                    }
                    else
                    {
                        *out++ = (char)ControlCodes::inline_sprite_str;
                        int32_t sprite_id = std::atoi(commands[1].data());
                        *((uint32_t*)out) = sprite_id;
                        out += 4;
                    }
                }
                else if (commands[0] == "INT32")
                {
                    if (commands.size() == 2 && commands[1] == "RAW")
                    {
                        *out++ = (char)ControlCodes::int32_ungrouped;
                    }
                    else
                    {
                        *out++ = (char)ControlCodes::int32_grouped;
                    }
                }
                else if (commands[0] == "RAWDATE" && commands.size() >= 2)
                {
                    *out++ = (char)ControlCodes::date;
                    if (commands.size() == 3 && commands[1] == "MY" && commands[2] == "SHORT")
                    {
                        *out++ = DateModifier::raw_my_abbr;
                    }
                }
                else if (commands[0] == "DATE" && commands.size() == 2)
                {
                    *out++ = (char)ControlCodes::date;
                    if (commands[1] == "DMY")
                    {
                        *out++ = DateModifier::dmy_full;
                    }
                    else if (commands[1] == "MY")
                    {
                        *out++ = DateModifier::my_full;
                    }
                }
                else if (commands[0] == "COLOUR")
                {
                    auto colour = textColourNames.find(commands[1]);
                    if (colour != textColourNames.end())
                    {
                        *out = colour->second;
                        out++;
                    }
                    else
                    {
                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "MOVE_X")
                {
                    *out++ = (char)ControlCodes::move_x;
                    uint8_t pixels_to_move_by = std::atoi(commands[1].data());
                    *out++ = pixels_to_move_by;
                }
                else if (commands[0] == "NEWLINE")
                {
                    if (commands.size() == 1)
                    {
                        *out++ = (char)ControlCodes::newline;
                    }
                    else if (commands.size() == 3)
                    {
                        *out++ = (char)ControlCodes::newline_x_y;

                        uint8_t x_pixels_to_move_by = std::atoi(commands[1].data());
                        *out++ = x_pixels_to_move_by;

                        uint8_t y_pixels_to_move_by = std::atoi(commands[2].data());
                        *out++ = y_pixels_to_move_by;
                    }
                }
                else
                {
                    printf("%.*s\n", (int)commands[0].length(), commands[0].data());
                }

                continue;
            }
            else
            {
                *out = readChar;
                out++;
            }

            if (readChar == '\0')
                break;
        }

        return str;
    }

    static bool stringIsBuffer(int id)
    {
        return id == StringIds::buffer_337 || id == StringIds::buffer_338 || id == StringIds::buffer_1250 || id == StringIds::preferred_currency_buffer || id == StringIds::buffer_1719
            || id == StringIds::buffer_2039 || id == StringIds::buffer_2040 || id == StringIds::buffer_2042 || id == StringIds::buffer_2045;
    }

    static bool loadLanguageStringTable(fs::path languageFile)
    {
        try
        {
            YAML::Node node = YAML::LoadFile(languageFile.string());
            node = node["strings"];

            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
            {
                int id = it->first.as<int>();
                if (stringIsBuffer(id))
                    continue;

                std::string new_string = it->second.as<std::string>();
                _strings_owner.emplace_back(readString(new_string.data(), new_string.length()));
                char* processed_string = _strings_owner.back().get();

                if (processed_string != nullptr)
                {
                    _strings[id] = processed_string;
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << "\n";
            return false;
        }
    }

    void loadLanguageFile()
    {
        // First, load en-GB for fallback strings.
        fs::path languageDir = Environment::getPath(Environment::path_id::language_files);
        fs::path languageFile = languageDir / "en-GB.yml";
        if (!loadLanguageStringTable(languageFile))
            throw std::runtime_error("Could not load the en-GB language file!");

        // Determine the language currently selected.
        auto& config = Config::getNew();
        if (config.language == "en-GB")
            return;

        // Now, load the language table for the language currently selected.
        languageFile = languageDir / (config.language + ".yml");
        if (!loadLanguageStringTable(languageFile))
            throw std::runtime_error("Could not load the " + config.language + " language file!");
    }

    void unloadLanguageFile()
    {
        _strings_owner.clear();
    }
}
