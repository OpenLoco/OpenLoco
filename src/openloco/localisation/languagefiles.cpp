#include "languagefiles.h"
#include "../interop/interop.hpp"
#include "../utility/yaml.hpp"
#include "stringmgr.h"
#include <cassert>

using namespace openloco::interop;

namespace openloco::localisation
{
    static loco_global<char * [0xFFFF], 0x005183FC> _strings;

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

                if (commands[0] == "STRINGID")
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
                else if (commands[0] == "CURRENCY48")
                {
                    *out = (char)control_codes::currency48;
                    out++;
                }
                else if (commands[0] == "STRING")
                {
                    *out = (char)control_codes::string_ptr;
                    out++;
                }
                else if (commands[0] == "POP16")
                {
                    *out = (char)control_codes::pop16;
                    out++;
                }
                else if (commands[0] == "POWER")
                {
                    *out = (char)control_codes::power;
                    out++;
                }
                else if (commands[0] == "HEIGHT")
                {
                    *out = (char)control_codes::height;
                    out++;
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
                else if (commands[0] == "CURRENCY32")
                {
                    *out = (char)control_codes::currency32;
                    out++;
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
                else if (commands[0] == "VELOCITY")
                {
                    *out = (char)control_codes::velocity;
                    out++;
                }
                else if (commands[0] == "DATE")
                {
                }
                else if (commands[0] == "DOWN")
                {
                }
                else if (commands[0] == "UP")
                {
                }
                else if (commands[0] == "SYMBOL_RAILWAY")
                {
                }
                else if (commands[0] == "SYMBOL_ROAD")
                {
                }
                else if (commands[0] == "SYMBOL_AIR")
                {
                }
                else if (commands[0] == "SYMBOL_WATER")
                {
                }
                else if (commands[0] == "CROSS")
                {
                }
                else if (commands[0] == "TICK")
                {
                }
                else if (commands[0] == "SMALLUP")
                {
                }
                else if (commands[0] == "SMALLDOWN")
                {
                }
                else if (commands[0] == "RIGHT")
                {
                }
                else if (commands[0] == "COLOUR")
                {
                    if (commands[1] == "BLACK")
                    {
                        *out = control_codes::colour_black;
                        out++;
                    }
                    else if (commands[1] == "WINDOW_1")
                    {
                        *out = control_codes::window_colour_1;
                        out++;
                    }
                    else if (commands[1] == "WINDOW_2")
                    {
                        *out = control_codes::window_colour_2;
                        out++;
                    }
                    else if (commands[1] == "WINDOW_3")
                    {
                        *out = control_codes::window_colour_3;
                        out++;
                    }
                    else if (commands[1] == "WHITE")
                    {
                        *out = control_codes::colour_white;
                        out++;
                    }
                    else if (commands[1] == "YELLOW")
                    {
                        *out = control_codes::colour_yellow;
                        out++;
                    }
                    else if (commands[1] == "TOPAZ")
                    {
                        *out = control_codes::colour_topaz;
                        out++;
                    }
                    else if (commands[1] == "RED")
                    {
                        *out = control_codes::colour_red;
                        out++;
                    }
                    else if (commands[1] == "GREEN")
                    {
                        *out = control_codes::colour_green;
                        out++;
                    }
                    else
                    {
                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "SMALLFONT")
                {
                    *out = (char)control_codes::font_regular;
                    out++;
                }
                else if (commands[0] == "BIGFONT")
                {
                    *out = (char)control_codes::font_large;
                    out++;
                }
                else if (commands[0] == "TINYFONT")
                {
                    *out = (char)control_codes::font_small;
                    out++;
                }
                else if (commands[0] == "MOVE_X")
                {
                }
                else if (commands[0] == "NEWLINE_SMALLER")
                {
                    *out = (char)control_codes::newline_smaller;
                    out++;
                }
                else if (commands[0] == "NEWLINE")
                {
                    *out = (char)control_codes::newline;
                    out++;
                }
                else if (commands[0] == "OUTLINE")
                {
                    *out = openloco::control_codes::outline;
                    out++;
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
        YAML::Node node = YAML::LoadFile("/home/aaron/de-DE.yml");
        [[maybe_unused]] auto type = node.Type();
        for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
        {
            int key2 = it->first.as<int>();

            if (key2 == 337 || key2 == 338 || key2 == 1250 || key2 == 1506 || key2 == 1719
                || key2 == 2039 || key2 == 2040 || key2 == 2042 || key2 == 2045)
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
