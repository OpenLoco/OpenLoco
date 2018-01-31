#include "messagemgr.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::messagemgr
{
    void post(
        message_type type,
        company_id_t companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC)
    {
        registers regs;
        regs.al = (uint8_t)type;
        regs.ah = companyId;
        regs.bx = subjectIdA;
        regs.cx = subjectIdB;
        regs.dx = subjectIdC;
        call(0x004285BA, regs);
    }
}
