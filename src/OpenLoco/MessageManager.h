#pragma once

#include "Company.h"
#include "Message.h"
#include <cstdint>

namespace OpenLoco
{
    enum class messageType
    {
        industryClosingDown = 1,
        cargoNowAccepted = 9,
        cargoNoLongerAccepted = 10,
        newCompany,
        citizensCelebrate = 13,
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
}

namespace OpenLoco::MessageManager
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
