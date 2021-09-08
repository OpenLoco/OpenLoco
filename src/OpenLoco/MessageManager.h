#pragma once
#include "Message.h"
#include <cstddef>

namespace OpenLoco::MessageManager
{
    Message* get(MessageId_t id);

    // 0x004285BA
    // al: type
    // ah: companyId
    // bx: subjectId A (station)
    // cx: subjectId B (cargo)
    // dx: subjectId C
    void post(
        MessageType type,
        CompanyId_t companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC = 0xFFFF);

    void updateDaily();

    void removeRelatedMessages(const EntityId_t id, const uint8_t type);
}
