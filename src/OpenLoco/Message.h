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

#pragma pack(push, 1)
    struct Message
    {
        static constexpr uint8_t kNumSubjects = 3;

        MessageType type;                    // 0x00
        char messageString[198];             // 0x01
        CompanyId companyId;                 // 0xC7
        uint16_t timeActive;                 // 0xC8 (1 << 15) implies manually opened news 0xFFFF implies no longer active
        uint16_t itemSubjects[kNumSubjects]; // 0xCA
        uint32_t date;                       // 0xD0

        constexpr bool operator==(const Message& rhs) const
        {
            // Unsure why companyId not compared
            return type == rhs.type
                && itemSubjects[0] == rhs.itemSubjects[0]
                && itemSubjects[1] == rhs.itemSubjects[1]
                && itemSubjects[2] == rhs.itemSubjects[2];
        }
        constexpr bool operator!=(const Message& rhs) const { return !(*this == rhs); }
        constexpr bool isActive() const { return timeActive != 0xFFFF; }
        constexpr bool isUserSelected() const { return timeActive & (1 << 15); }
        constexpr void setActive(bool state)
        {
            if (state)
            {
                timeActive = 0;
            }
            else
            {
                timeActive = 0xFFFF;
            }
        }
        constexpr void setUserSelected()
        {
            setActive(true);
            timeActive |= (1 << 15);
        }
    };
#pragma pack(pop)

    struct MessageTypeDescriptor
    {
        MessageCriticality criticality;
        Audio::SoundId sound;
        uint16_t flags;
        MessageItemArgumentType argumentTypes[Message::kNumSubjects];
        uint16_t duration;
        uint8_t priority;
        constexpr bool hasFlag(uint16_t flag) const { return flags & flag; }
    };

    const MessageTypeDescriptor& getMessageTypeDescriptor(const MessageType type);
}
