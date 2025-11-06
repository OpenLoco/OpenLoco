#include <cstdint>

namespace OpenLoco
{
    struct Message;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Message
    {
        uint8_t type;             // 0x00
        char messageString[198];  // 0x01
        uint8_t companyId;        // 0xC7
        uint16_t timeActive;      // 0xC8 (1 << 15) implies manually opened news 0xFFFF implies no longer active
        uint16_t itemSubjects[3]; // 0xCA
        uint32_t date;            // 0xD0
    };
    static_assert(sizeof(Message) == 0xD4);
#pragma pack(pop)

    
    S5::Message exportMessage(OpenLoco::Message& src);
}
