#pragma once

#include "Types.hpp"
#include <limits>

namespace OpenLoco::Audio
{
    enum class SoundId : uint16_t;
}

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
        companyCheated,
        newSpeedRecord = 29,
    };

    enum class MessageCriticality : uint8_t
    {
        majorCompany,
        majorCompetitor,
        minorCompany,
        minorCompetitor,
        general,
        advice,
    };

    enum class MessageItemArgumentType : uint8_t
    {
        industry,
        station,
        town,
        vehicle,
        company,
        location,
        unk6,
        vehicleTab = 7,
        null = std::numeric_limits<uint8_t>::max()
    };

    namespace MessageTypeFlags
    {
        constexpr uint16_t unk0 = 1 << 0;
        constexpr uint16_t unk1 = 1 << 1;
        constexpr uint16_t hasFirstItem = 1 << 2;
        constexpr uint16_t hasSecondItem = 1 << 3;
        constexpr uint16_t unk5 = 1 << 5; // Never set
    }

    struct MessageTypeDescriptor
    {
        MessageCriticality criticality;
        Audio::SoundId sound;
        uint16_t flags;
        MessageItemArgumentType argumentTypes[3];
        constexpr bool hasFlag(uint16_t flag) const { return flags & flag; }
    };

    const MessageTypeDescriptor& getMessageTypeDescriptor(const MessageType type);

#pragma pack(push, 1)
    struct Message
    {
        MessageType type;        // 0x00
        char messageString[198]; // 0x01
        CompanyId companyId;     // 0xC7
        uint16_t var_C8;
        uint16_t itemSubjects[3]; // 0xCA
        uint32_t date; // 0xD0
    };
#pragma pack(pop)
}
