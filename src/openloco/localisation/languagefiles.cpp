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
                }
                else if (commands[0] == "STRING")
                {
                }
                else if (commands[0] == "POP16")
                {
                    *out = (char)(123 + 13);
                    out++;
                }
                else if (commands[0] == "POWER")
                {
                }
                else if (commands[0] == "HEIGHT")
                {
                }
                else if (commands[0] == "UINT16")
                {
                }
                else if (commands[0] == "SPRITE")
                {
                    if (commands.size() == 1)
                    {
                        *out = (char)(123 + 20);
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
                    }
                    else if (commands[1] == "WINDOW_1")
                    {
                        *out = openloco::control_code::window_colour_1;
                        out++;
                    }
                    else if (commands[1] == "WINDOW_2")
                    {
                        *out = openloco::control_code::window_colour_2;
                        out++;
                    }
                    else if (commands[1] == "WINDOW_3")
                    {
                        *out = openloco::control_code::window_colour_3;
                        out++;
                    }
                    else if (commands[1] == "WHITE")
                    {
                    }
                    else if (commands[1] == "YELLOW")
                    {
                    }
                    else if (commands[1] == "TOPAZ")
                    {
                    }
                    else if (commands[1] == "RED")
                    {
                    }
                    else if (commands[1] == "GREEN")
                    {
                    }
                    else
                    {
                        printf("%.*s\n", (int)commands[1].length(), commands[1].data());
                    }
                }
                else if (commands[0] == "SMALLFONT")
                {
                }
                else if (commands[0] == "BIGFONT")
                {
                }
                else if (commands[0] == "TINYFONT")
                {
                }
                else if (commands[0] == "MOVE_X")
                {
                }
                else if (commands[0] == "NEWLINE_SMALLER")
                {
                }
                else if (commands[0] == "NEWLINE")
                {
                }
                else if (commands[0] == "OUTLINE")
                {
                    *out = openloco::control_code::outline;
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
        YAML::Node node = YAML::LoadFile("/Users/Marijn/de-DE.yml");
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

            auto str = readString(str2.data(), str2.length());

            if (str != nullptr)
            {
                _strings[key2] = str;
                printf("%4d |%s|\n", key2, str);
            }
        }

        return true;
    }
}
