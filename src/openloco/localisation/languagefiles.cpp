#include "languagefiles.h"
#include "../config.h"
#include "../console.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../platform/platform.h"
#include "../utility/yaml.hpp"
#include "string_ids.h"
#include "stringmgr.h"
#include <cassert>
#include <iostream>

using namespace openloco::interop;

namespace openloco::localisation
{
    static loco_global<char * [0xFFFF], 0x005183FC> _strings;

    static std::map<std::string, uint8_t, std::less<>> basicCommands = {
        { "SMALLFONT", control_codes::font_regular },
        { "BIGFONT", control_codes::font_large },
        { "TINYFONT", control_codes::font_small },
        { "NEWLINE_SMALLER", control_codes::newline_smaller },
        { "NEWLINE", control_codes::newline },
        { "OUTLINE", openloco::control_codes::outline },
        { "CROSS", 0x41 },
        { "TICK", 0x41 },
        { "SMALLUP", 0x41 },
        { "SMALLDOWN", 0x41 },
        { "RIGHT", 0x41 },
        { "DOWN", 0x41 },
        { "UP", 0x41 },
        { "SYMBOL_RAILWAY", 0x41 },
        { "SYMBOL_ROAD", 0x41 },
        { "SYMBOL_AIR", 0x41 },
        { "SYMBOL_WATER", 0x41 },
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

    static uint32_t readCodePoint(uint8_t** string)
    {
        uint32_t read = 0;

        uint8_t* ptr = *string;

        if ((ptr[0] & 0b10000000) == 0)
        {
            read = ptr[0];
            *string += 1;
        }
        else if ((ptr[0] & 0b11100000) == 0b11000000)
        {
            assert((ptr[1] & 0b11000000) == 0b10000000);

            read = (ptr[0] & 0b11111) << 6 | ((ptr[1] & 0b111111));
            //printf("%02X %02X %d %s\n", ptr[0], ptr[1], read, value);
            *string += 2;
        }
        else if ((ptr[0] & 0b11110000) == 0b11100000)
        {
            assert((ptr[1] & 0b11000000) == 0b10000000);
            assert((ptr[2] & 0b11000000) == 0b10000000);
            *string += 3;
        }
        else if ((ptr[0] & 0b11111000) == 0b11110000)
        {
            *string += 4;
        }

        return read;
    }

    [[maybe_unused]] static char* readString(const char* value, size_t size)
    {
        // Take terminating NULL character in account
        char* str = (char*)malloc(size + 1);
        char* out = str;

        uint8_t* ptr = (uint8_t*)value;
        while (true)
        {
            uint32_t readChar = readCodePoint(&ptr);
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
                            commands.push_back(std::basic_string_view(start, pos - start));
                        }
                        start = nullptr;
                        continue;
                    }

                    if (readChar == '}')
                    {
                        if (start != nullptr)
                        {
                            commands.push_back(std::basic_string_view(start, pos - start));
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
                        *out = (char)(123 + 8);
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
                        *out = (char)control_codes::inline_sprite_str;
                        out++;
                    }
                    else
                    {
                        char str[125] = { 0 };
                        memcpy(str, commands[1].data(), commands[1].length());
                        int32_t val = atoi(str);

                        *((uint32_t*)str) = val;

                        out += 4;

                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "NUM16")
                {
                }
                else if (commands[0] == "INT32")
                {
                }
                else if (commands[0] == "RAWDATE")
                {
                }
                else if (commands[0] == "DATE")
                {
                }
                else if (commands[0] == "COLOUR")
                {
                    auto search = textColourNames.find(commands[1]);
                    if (search != textColourNames.end())
                    {
                        *out = search->second;
                        out++;
                    }
                    else
                    {
                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "MOVE_X")
                {
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

    bool loadLanguageFile()
    {
        auto& config = config::get_new();
        fs::path languageDir = platform::GetCurrentExecutablePath().parent_path() / "data/language";
        fs::path languageFile = languageDir / (config.language + ".yml");

        if (!fs::exists(languageFile))
        {
            std::cerr << "Language file " << config.language << ".yml does not exist! ";
            if (config.language == "en-GB")
            {
                std::cerr << "Unable to load...\n";
                return false;
            }

            std::cerr << "Falling back to en-GB...\n";
            config.language = "en-GB";
            languageFile = languageDir / (config.language + ".yml");
            if (!fs::exists(languageFile))
            {
                std::cerr << "Language file " << config.language << ".yml does not exist! Unable to load...\n";
                return false;
            }
        }

#ifdef _OPENLOCO_USE_BOOST_FS_
        YAML::Node node = YAML::LoadFile(languageFile.string());
#else
        YAML::Node node = YAML::LoadFile(languageFile);
#endif

        [[maybe_unused]] auto type = node.Type();
        for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
        {
            int key2 = it->first.as<int>();

            if (key2 == 337 || key2 == 338 || key2 == 1250 || key2 == string_ids::preferred_currency_buffer || key2 == 1719
                || key2 == string_ids::buffer_2039 || key2 == string_ids::buffer_2040 || key2 == string_ids::buffer_2042 || key2 == string_ids::buffer_2045)
            {
                continue;
            }

            std::string str2 = it->second.as<std::string>();

            char* str = readString(str2.data(), str2.length());

            if (str != nullptr)
            {
                _strings[key2] = str;
                printf("%4d |%s|\n", key2, str);
            }
        }

        return true;
    }
}
