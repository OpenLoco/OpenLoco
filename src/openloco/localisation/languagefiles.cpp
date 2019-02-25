#include "languagefiles.h"
#include "../config.h"
#include "../console.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../platform/platform.h"
#include "../utility/yaml.hpp"
#include "conversion.h"
#include "string_ids.h"
#include "stringmgr.h"
#include "unicode.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace openloco::interop;

namespace openloco::localisation
{
    static loco_global<char * [0xFFFF], 0x005183FC> _strings;

    static std::map<std::string, uint8_t, std::less<>> basicCommands = {
        { "INT16_1DP", control_codes::int16_decimals },
        { "INT32_1DP", control_codes::int32_decimals },
        { "INT16", control_codes::int16_grouped },
        { "UINT16", control_codes::uint16_ungrouped },
        { "SMALLFONT", control_codes::font_regular },
        { "BIGFONT", control_codes::font_large },
        { "TINYFONT", control_codes::font_small },
        { "NEWLINE_SMALLER", control_codes::newline_smaller },
        { "OUTLINE", openloco::control_codes::outline },
        { "VELOCITY", control_codes::velocity },
        { "CURRENCY32", control_codes::currency32 },
        { "HEIGHT", control_codes::height },
        { "CURRENCY48", control_codes::currency48 },
        { "STRING", control_codes::string_ptr },
        { "POP16", control_codes::pop16 },
        { "POWER", control_codes::power },
    };

    static std::map<std::string, uint8_t, std::less<>> textColourNames = {
        { "BLACK", control_codes::colour_black },
        { "WINDOW_1", control_codes::window_colour_1 },
        { "WINDOW_2", control_codes::window_colour_2 },
        { "WINDOW_3", control_codes::window_colour_3 },
        { "WHITE", control_codes::colour_white },
        { "YELLOW", control_codes::colour_yellow },
        { "TOPAZ", control_codes::colour_topaz },
        { "RED", control_codes::colour_red },
        { "GREEN", control_codes::colour_green },
    };

    static char* readString(const char* value, size_t size)
    {
        // Take terminating NULL character in account
        char* str = (char*)malloc(size + 1);
        char* out = str;

        utf8_t* ptr = (utf8_t*)value;
        while (true)
        {
            utf32_t codepoint = readCodePoint(&ptr);
            if (codepoint == unicode_char::superscript_minus || codepoint == unicode_char::variation_selector)
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
                        *out = (unsigned char)(123 + 8);
                        out++;
                    }
                    else
                    {
                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "UINT16")
                {
                    *out = (char)control_codes::uint16_ungrouped;
                    out++;
                }
                else if (commands[0] == "SPRITE")
                {
                    if (commands.size() == 1)
                    {
                        *out++ = (char)control_codes::inline_sprite_args;
                    }
                    else
                    {
                        *out++ = (char)control_codes::inline_sprite_str;
                        int32_t sprite_id = std::atoi(commands[1].data());
                        *((uint32_t*)out) = sprite_id;
                        out += 4;
                    }
                }
                else if (commands[0] == "INT32")
                {
                    if (commands.size() == 2 && commands[1] == "RAW")
                    {
                        *out++ = (char)control_codes::int32_ungrouped;
                    }
                    else
                    {
                        *out++ = (char)control_codes::int32_grouped;
                    }
                }
                else if (commands[0] == "RAWDATE" && commands.size() >= 2)
                {
                    *out++ = (char)control_codes::date;
                    if (commands.size() == 3 && commands[1] == "MY" && commands[2] == "SHORT")
                    {
                        *out++ = date_modifier::raw_my_abbr;
                    }
                }
                else if (commands[0] == "DATE" && commands.size() == 2)
                {
                    *out++ = (char)control_codes::date;
                    if (commands[1] == "DMY")
                    {
                        *out++ = date_modifier::dmy_full;
                    }
                    else if (commands[1] == "MY")
                    {
                        *out++ = date_modifier::my_full;
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
                    *out++ = (char)control_codes::move_x;
                    uint8_t pixels_to_move_by = std::atoi(commands[1].data());
                    *out++ = pixels_to_move_by;
                }
                else if (commands[0] == "NEWLINE")
                {
                    if (commands.size() == 1)
                    {
                        *out++ = (char)control_codes::newline;
                    }
                    else if (commands.size() == 3)
                    {
                        *out++ = (char)control_codes::newline_x_y;

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
        return id == string_ids::buffer_337 || id == string_ids::buffer_338 || id == string_ids::buffer_1250 || id == string_ids::preferred_currency_buffer || id == string_ids::buffer_1719
            || id == string_ids::buffer_2039 || id == string_ids::buffer_2040 || id == string_ids::buffer_2042 || id == string_ids::buffer_2045;
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
                char* processed_string = readString(new_string.data(), new_string.length());

                if (processed_string != nullptr)
                    _strings[id] = processed_string;
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
        fs::path languageDir = environment::get_path(environment::path_id::language_files);
        fs::path languageFile = languageDir / "en-GB.yml";
        if (!loadLanguageStringTable(languageFile))
            throw std::runtime_error("Could not load the en-GB language file!");

        // Determine the language currently selected.
        auto& config = config::get_new();
        if (config.language == "en-GB")
            return;

        // Now, load the language table for the language currently selected.
        languageFile = languageDir / (config.language + ".yml");
        if (!loadLanguageStringTable(languageFile))
            throw std::runtime_error("Could not load the " + config.language + " language file!");
    }
}
