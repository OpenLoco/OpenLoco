#pragma once

#include "company.h"
#include <cstdint>

namespace openloco
{
    enum class message_type
    {
        cargo_now_accepted = 9,
        cargo_no_longer_accepted = 10,
        unable_to_land_at_airport = 12,
    };
}

namespace openloco::messagemgr
{

    // 0x004285BA
    // al: type
    // ah: companyId
    // bx: subjectId A (station)
    // cx: subjectId B (cargo)
    // dx: subjectId C
    void post(
        message_type type,
        company_id_t companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC = 0xFFFF);
}
