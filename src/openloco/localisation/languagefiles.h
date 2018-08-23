#include <cstdint>

namespace openloco::localisation
{
    uint32_t readCodePoint(uint8_t** string);
    bool loadLanguageFile();
}
