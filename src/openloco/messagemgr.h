#pragma once

#include "company.h"
#include "message.h"
#include <cstdint>

namespace openloco::messagemgr
{
    constexpr size_t max_messages = 199;

    message* get(message_id_t id);

    // 0x004285BA
    // al: type
    // ah: companyId
    // bx: subjectId A (station)
    // cx: subjectId B (cargo)
    // dx: subjectId C
    void post(
        messageType type,
        company_id_t companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC = 0xFFFF);
}
