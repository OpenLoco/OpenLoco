#pragma once

#include "Types.hpp"
#include <limits>

namespace OpenLoco
{
    enum class MessageType : uint8_t
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

    enum class NewsItemCriticality : uint8_t
    {
        majorCompany,
        majorCompetitor,
        minorCompany,
        minorCompetitor,
        general,
        advice,
    };

    enum class NewsItemSubTypes : uint8_t
    {
        industry,
        station,
        town,
        vehicle,
        company,
        unk5,
        unk6,
        vehicleTab = 7,
        null = std::numeric_limits<uint8_t>::max()
    };

    namespace MessageTypeFlags
    {
        constexpr uint16_t unk0 = 1 << 0;
        constexpr uint16_t unk1 = 1 << 1;
        constexpr uint16_t unk2 = 1 << 2;
        constexpr uint16_t unk3 = 1 << 3;
        constexpr uint16_t unk5 = 1 << 5; // Never set
    }

    struct MessageItemType
    {
        NewsItemSubTypes type[3]; // 0x00
    };

    bool hasMessageTypeFlag(const MessageType type, const uint16_t flag);
    const MessageItemType& getMessageCategories(const MessageType type);

#pragma pack(push, 1)
    struct Message
    {
        MessageType type;        // 0x00
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
