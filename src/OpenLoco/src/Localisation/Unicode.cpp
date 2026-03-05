#include "Unicode.h"
#include <cstdint>
#include <OpenLoco/Core/Exception.hpp>

namespace OpenLoco::Localisation
{
    utf32_t readCodePoint(const utf8_t** string)
    {
        utf32_t read = 0;

        const utf8_t* ptr = *string;

        if ((ptr[0] & 0b10000000) == 0)
        {
            read = ptr[0];
            *string += 1;
        }
        else if ((ptr[0] & 0b11100000) == 0b11000000)
        {
            read = ((ptr[0] & 0b11111) << 6) | (ptr[1] & 0b111111);
            *string += 2;
        }
        else if ((ptr[0] & 0b11110000) == 0b11100000)
        {
            read = ((ptr[0] & 0b1111) << 12) | ((ptr[1] & 0b111111) << 6) | (ptr[2] & 0b111111);
            *string += 3;
        }
        else if ((ptr[0] & 0b11111000) == 0b11110000)
        {
            read = ((ptr[0] & 0b111) << 18) | ((ptr[1] & 0b111111) << 12) | ((ptr[2] & 0b111111) << 6)
                | (ptr[3] & 0b111111);
            *string += 4;
        }

        return read;
    }

    std::string codepointToUtf8(utf32_t codepoint)
    {
        std::string string;
        if (codepoint < 0x80)
        {
            string += codepoint;
        }
        else if (codepoint < 0x800)
        {
            string += 0b11000000 + ((codepoint & 0b111111000000) >> 6);
            string += 0b10000000 + (codepoint & 0b111111);
        }
        else if (codepoint < 0x10000)
        {
            string += 0b11100000 + ((codepoint & 0b1111000000000000) >> 12);
            string += 0b10000000 + ((codepoint & 0b1111111000000) >> 6);
            string += 0b10000000 + (codepoint & 0b111111);
        }
        else if (codepoint < 110000)
        {
            string += 0b11100000 + ((codepoint & 0b111000000000000000000) >> 18);
            string += 0b10000000 + ((codepoint & 0b111111000000000000) >> 12);
            string += 0b10000000 + ((codepoint & 0b1111111000000) >> 6);
            string += 0b10000000 + (codepoint & 0b111111);
        }
        else
        {
            // Invalid
            string += "�";
        }
        return string;
    }

    size_t utf8Length(const std::string& string)
    {
        size_t count = 0;
        size_t i = 0;
        while (i < string.length())
        {
            if ((string[i] & 0b10000000) == 0)
            {
                i += 1;
            }
            else if ((string[i] & 0b11100000) == 0b11000000)
            {
                i += 2;
            }
            else if ((string[i] & 0b11110000) == 0b11100000)
            {
                i += 3;
            }
            else if ((string[i] & 0b11111000) == 0b11110000)
            {
                i += 4;
            }
            else
            {
                throw Exception::RuntimeError("Invalid UTF-8 string");
            }
            count++;
        }
        return count;
    }

    void utf8Insert(std::string& stringToModify, size_t characterPosition, const std::string& insertion)
    {
        // Get byte position
        size_t i = 0;
        for (size_t j = 0; j < characterPosition; j++)
        {
            if ((stringToModify[i] & 0b10000000) == 0)
            {
                i += 1;
            }
            else if ((stringToModify[i] & 0b11100000) == 0b11000000)
            {
                i += 2;
            }
            else if ((stringToModify[i] & 0b11110000) == 0b11100000)
            {
                i += 3;
            }
            else if ((stringToModify[i] & 0b11111000) == 0b11110000)
            {
                i += 4;
            }
            else
            {
                throw Exception::RuntimeError("Invalid UTF-8 string");
            }
        }

        stringToModify.insert(i, insertion);
    }

    void utf8Delete(std::string& stringToModify, size_t characterPosition)
    {
        // Get byte position
        size_t i = 0;
        for (size_t j = 0; j < characterPosition; j++)
        {
            if ((stringToModify[i] & 0b10000000) == 0)
            {
                i += 1;
            }
            else if ((stringToModify[i] & 0b11100000) == 0b11000000)
            {
                i += 2;
            }
            else if ((stringToModify[i] & 0b11110000) == 0b11100000)
            {
                i += 3;
            }
            else if ((stringToModify[i] & 0b11111000) == 0b11110000)
            {
                i += 4;
            }
            else
            {
                throw Exception::RuntimeError("Invalid UTF-8 string");
            }
        }

        // Get length of target character
        uint8_t bytes = 0;
        if ((stringToModify[i] & 0b10000000) == 0)
        {
            bytes = 1;
        }
        else if ((stringToModify[i] & 0b11100000) == 0b11000000)
        {
            bytes = 2;
        }
        else if ((stringToModify[i] & 0b11110000) == 0b11100000)
        {
            bytes = 3;
        }
        else if ((stringToModify[i] & 0b11111000) == 0b11110000)
        {
            bytes = 4;
        }

        stringToModify.erase(i, bytes);
    }
}
