#pragma once

#include "Types.hpp"
#include <limits>

namespace OpenLoco
{
    namespace MessageId
    {
        constexpr MessageId_t null = std::numeric_limits<MessageId_t>::max();
    }

    enum class MessageType
    {
        cantWaitForFullLoad = 0,
        industryClosingDown = 1,
        cargoNowAccepted = 9,
        cargoNoLongerAccepted = 10,
        newCompany,
        unableToLandAtAirport = 12,
        citizensCelebrate,
        workersCelebrate,
        newVehicle,
        newIndustry = 17,
        industryProductionUp,
        industryProductionDown,
        bankruptcyWarning6Months = 23,
        bankruptcyWarning3Months,
        bankruptcyDeclared,
        vehicleCrashed = 27,
        newSpeedRecord = 29,
    };

    enum class NewsItemSubType : uint8_t
    {
        majorCompany,
        majorCompetitor,
        minorCompany,
        minorCompetitor,
        general,
        advice,
    };

#pragma pack(push, 1)
    struct Message
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
