#include "LanguageFiles.h"
#include "Config.h"
#include "Conversion.h"
#include "Environment.h"
#include "Localisation/Formatting.h"
#include "Logging.h"
#include "StringIds.h"
#include "StringManager.h"
#include "Ui.h"
#include "Unicode.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

using namespace OpenLoco::Interop;

namespace OpenLoco::Localisation
{
    static loco_global<char* [0xFFFF], 0x005183FC> _strings;
    static std::vector<std::unique_ptr<char[]>> _stringsOwner;

    static const std::map<std::string, uint8_t, std::less<>> kBasicCommands = {
        { "INT16_1DP", ControlCodes::int16_decimals },
        { "INT32_1DP", ControlCodes::int32_decimals },
        { "INT16", ControlCodes::int16_grouped },
        { "UINT16", ControlCodes::uint16_ungrouped },
        { "SMALLFONT", ControlCodes::Font::regular },
        { "BIGFONT", ControlCodes::Font::large },
        { "TINYFONT", ControlCodes::Font::small },
        { "NEWLINE_SMALLER", ControlCodes::newlineSmaller },
        { "OUTLINE", OpenLoco::ControlCodes::Font::outline },
        { "VELOCITY", ControlCodes::velocity },
        { "CURRENCY32", ControlCodes::currency32 },
        { "HEIGHT", ControlCodes::height },
        { "CURRENCY48", ControlCodes::currency48 },
        { "STRING", ControlCodes::string_ptr },
        { "POP16", ControlCodes::pop16 },
        { "POWER", ControlCodes::power },
    };

    static const std::map<std::string, uint8_t, std::less<>> kTextColourNames = {
        { "BLACK", ControlCodes::Colour::black },
        { "WINDOW_1", ControlCodes::windowColour1 },
        { "WINDOW_2", ControlCodes::windowColour2 },
        { "WINDOW_3", ControlCodes::windowColour3 },
        { "WINDOW_4", ControlCodes::windowColour4 },
        { "WHITE", ControlCodes::Colour::white },
        { "YELLOW", ControlCodes::Colour::yellow },
        { "TOPAZ", ControlCodes::Colour::topaz },
        { "RED", ControlCodes::Colour::red },
        { "GREEN", ControlCodes::Colour::green },
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

                auto search = kBasicCommands.find(commands[0]);
                if (search != kBasicCommands.end())
                {
                    *out = search->second;
                    out++;
                }
                else if (commands[0] == "STRINGID")
                {
                    if (commands.size() == 1)
                    {
                        *out = (char)ControlCodes::stringidArgs;
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
                        *out++ = (char)ControlCodes::inlineSpriteArgs;
                    }
                    else
                    {
                        *out++ = (char)ControlCodes::inlineSpriteStr;
                        int32_t spriteId = std::atoi(commands[1].data());
                        *((uint32_t*)out) = spriteId;
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
                    auto colour = kTextColourNames.find(commands[1]);
                    if (colour != kTextColourNames.end())
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
                    *out++ = (char)ControlCodes::moveX;
                    uint8_t pixelsToMoveBy = std::atoi(commands[1].data());
                    *out++ = pixelsToMoveBy;
                }
                else if (commands[0] == "NEWLINE")
                {
                    if (commands.size() == 1)
                    {
                        *out++ = (char)ControlCodes::newline;
                    }
                    else if (commands.size() == 3)
                    {
                        *out++ = (char)ControlCodes::newlineXY;

                        uint8_t xPixelsToMoveBy = std::atoi(commands[1].data());
                        *out++ = xPixelsToMoveBy;

                        uint8_t yPixelsToMoveBy = std::atoi(commands[2].data());
                        *out++ = yPixelsToMoveBy;
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
                _stringsOwner.emplace_back(readString(new_string.data(), new_string.length()));
                char* processedString = _stringsOwner.back().get();

                if (processedString != nullptr)
                {
                    _strings[id] = processedString;
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << "\n";
            Ui::showMessageBox("Exception", e.what());
            return false;
        }
    }

    void loadLanguageFile()
    {
        // First, load en-GB for fallback strings.
        fs::path languageDir = Environment::getPath(Environment::PathId::languageFiles);
        fs::path languageFile = languageDir / "en-GB.yml";
        if (!loadLanguageStringTable(languageFile))
            throw std::runtime_error("Could not load the en-GB language file!");

        // Determine the language currently selected.
        auto& config = Config::get();
        if (config.language == "en-GB")
            return;

        // Now, load the language table for the language currently selected.
        languageFile = languageDir / (config.language + ".yml");
        if (!loadLanguageStringTable(languageFile))
            throw std::runtime_error("Could not load the " + config.language + " language file!");
    }

    void unloadLanguageFile()
    {
        _stringsOwner.clear();
    }
}
