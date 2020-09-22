#include "MessageManager.h"
#include "Interop/Interop.hpp"
#include "OpenLoco.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::messagemgr
{
    static loco_global<message[max_messages], 0x005271D2> _messages;

    message* get(message_id_t id)
    {
        if (id >= _messages.size())
        {
            return nullptr;
        }
        return &_messages[id];
    }

    void post(
        messageType type,
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
