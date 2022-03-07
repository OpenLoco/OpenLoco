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
        cantWaitForFullLoad,
        industryClosingDown,
        vehicleSlipped,
        unk3,
        unk4,
        unk5,
        unk6,
        unk7,
        unk8,
        cargoNowAccepted,
        cargoNoLongerAccepted,
        newCompany,
        unableToLandAtAirport,
        citizensCelebrate,
        workersCelebrate,
        newVehicle,
        companyPromoted,
        newIndustry,
        industryProductionUp,
        industryProductionDown,
        congratulationsCompleted,
        failedObjectives,
        haveBeenBeaten,
        bankruptcyWarning6Months,
        bankruptcyWarning3Months,
        bankruptcyDeclared,
        bankruptcyDeclared2,
        vehicleCrashed,
        companyCheated,
        newSpeedRecord,
        newSpeedRecord2,
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
        uint16_t duration;
        uint8_t priority;
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
        uint32_t date;            // 0xD0

        constexpr bool operator==(const Message& lhs) const
        {
            // Unsure why companyId not compared
            return type == lhs.type
                && itemSubjects[0] == lhs.itemSubjects[0]
                && itemSubjects[1] == lhs.itemSubjects[1]
                && itemSubjects[2] == lhs.itemSubjects[2];
        }
        constexpr bool operator!=(const Message& lhs) const { return !(*this == lhs); }
    };
#pragma pack(pop)
}
