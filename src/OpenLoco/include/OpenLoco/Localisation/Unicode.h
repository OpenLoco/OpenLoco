#pragma once

#include <cstdint>
#include <string>

namespace OpenLoco::Localisation
{
    using utf8_t = uint8_t;
    using utf32_t = uint32_t;

    utf32_t readCodePoint(const utf8_t** string);
    std::string codepointToUtf8(utf32_t codepoint);
}
