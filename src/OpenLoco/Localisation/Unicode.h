#include <cstdint>

namespace openloco::localisation
{
    using utf8_t = uint8_t;
    using utf32_t = uint32_t;

    utf32_t readCodePoint(utf8_t** string);
}
