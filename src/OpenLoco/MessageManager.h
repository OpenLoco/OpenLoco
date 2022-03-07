#pragma once
#include "Message.h"
#include <cstddef>

namespace OpenLoco::MessageManager
{
    Message* get(MessageId id);

    // 0x004285BA
    // al: type
    // ah: companyId
    // bx: subjectId A (station)
    // cx: subjectId B (cargo)
    // dx: subjectId C
    void post(
        MessageType type,
        CompanyId companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC = 0xFFFF);

    void updateDaily();
}
