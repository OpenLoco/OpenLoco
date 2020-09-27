#pragma once

#include "Localisation/StringManager.h"
#include "Map/Tile.h"
#include <limits>

namespace OpenLoco
{

    using message_id_t = uint16_t;

    namespace MessageId
    {
        constexpr message_id_t null = std::numeric_limits<message_id_t>::max();
    }

    enum class newsItemSubType : uint8_t
    {
        majorCompany,
        majorCompetitor,
        minorCompany,
        minorCompetitor,
        general,
        advice,
    };

#pragma pack(push, 1)
    struct message
    {
        uint8_t type;            // 0x00
        char messageString[198]; // 0x01
        uint8_t companyId;       // 0xC7
        uint16_t var_C8;
        uint16_t item_id_1; // 0xCA
        uint16_t item_id_2; // 0xCC
        uint8_t pad_CE[0xD0 - 0xCE];
        uint32_t date; // 0xD0
    };
#pragma pack(pop)
}
