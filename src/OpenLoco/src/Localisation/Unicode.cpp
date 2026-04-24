#include "Unicode.h"
#include <OpenLoco/Core/Exception.hpp>
#include <cstdint>

namespace OpenLoco::Localisation
{
    utf32_t readCodePoint(const utf8_t** string)
    {
        utf32_t read = 0;

        const utf8_t* ptr = *string;

        if ((ptr[0] & 0b1000'0000) == 0)
        {
            read = ptr[0];
            *string += 1;
        }
        else if ((ptr[0] & 0b1110'0000) == 0b1100'0000)
        {
            read = ((ptr[0] & 0b1'1111) << 6) | (ptr[1] & 0b11'1111);
            *string += 2;
        }
        else if ((ptr[0] & 0b1111'0000) == 0b1110'0000)
        {
            read = ((ptr[0] & 0b1111) << 12) | ((ptr[1] & 0b11'1111) << 6) | (ptr[2] & 0b11'1111);
            *string += 3;
        }
        else if ((ptr[0] & 0b1111'1000) == 0b1111'0000)
        {
            read = ((ptr[0] & 0b111) << 18) | ((ptr[1] & 0b11'1111) << 12) | ((ptr[2] & 0b11'1111) << 6)
                | (ptr[3] & 0b11'1111);
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
            string += 0b1100'0000 + ((codepoint & 0b111'11100'0000) >> 6);
            string += 0b1000'0000 + (codepoint & 0b11'1111);
        }
        else if (codepoint < 0x10000)
        {
            string += 0b1110'0000 + ((codepoint & 0b1111'0000'0000'0000) >> 12);
            string += 0b1000'0000 + ((codepoint & 0b1'1111'1100'0000) >> 6);
            string += 0b1000'0000 + (codepoint & 0b11'1111);
        }
        else if (codepoint < 0x110000)
        {
            string += 0b1110'0000 + ((codepoint & 0b1'1100'0000'0000'0000'0000) >> 18);
            string += 0b1000'0000 + ((codepoint & 0b11'1111'0000'0000'0000) >> 12);
            string += 0b1000'0000 + ((codepoint & 0b1'1111'1100'0000) >> 6);
            string += 0b1000'0000 + (codepoint & 0b11'1111);
        }
        else
        {
            // Invalid
            string += "�";
        }
        return string;
    }

    static size_t utf8CharacterLength(const std::string& string, size_t cursor = 0)
    {
        if ((string[cursor] & 0b1000'0000) == 0)
        {
            return 1;
        }
        else if ((string[cursor] & 0b1110'0000) == 0b1100'0000)
        {
            return 2;
        }
        else if ((string[cursor] & 0b1111'0000) == 0b1110'0000)
        {
            return 3;
        }
        else if ((string[cursor] & 0b1111'1000) == 0b1111'0000)
        {
            return 4;
        }
        throw Exception::RuntimeError("Invalid UTF-8 string");
    }

    static size_t nextUtf8CodePointPosition(const std::string& string, size_t cursor)
    {
        return cursor + utf8CharacterLength(string, cursor);
    }

    static size_t utf8CharacterPosition(const std::string& str, size_t characterPosition)
    {
        size_t cursor = 0;
        for (size_t i = 0; i < characterPosition; i++)
        {
            cursor = nextUtf8CodePointPosition(str, cursor);
        }
        return cursor;
    }

    size_t utf8Length(const std::string& string)
    {
        size_t count = 0;
        size_t cursor = 0;

        while (cursor < string.length())
        {
            cursor = nextUtf8CodePointPosition(string, cursor);
            count++;
        }
        return count;
    }

    void utf8Insert(std::string& stringToModify, size_t characterPosition, const std::string& insertion)
    {
        auto cursor = utf8CharacterPosition(stringToModify, characterPosition);
        stringToModify.insert(cursor, insertion);
    }

    void utf8Delete(std::string& stringToModify, size_t characterPosition)
    {
        auto cursor = utf8CharacterPosition(stringToModify, characterPosition);
        auto length = utf8CharacterLength(stringToModify, cursor);
        stringToModify.erase(cursor, length);
    }
}
