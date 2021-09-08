#include "MessageManager.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "OpenLoco.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::MessageManager
{
    static auto& rawMessages() { return getGameState().messages; }

    Message* get(MessageId_t id)
    {
        if (id >= Limits::maxMessages)
        {
            return nullptr;
        }
        return &rawMessages()[id];
    }

    void post(
        MessageType type,
        CompanyId_t companyId,
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

    // 0x004284DB
    void updateDaily()
    {
        call(0x004284DB);
    }

    // 0x0042851C
    void removeRelatedMessages(const EntityId_t id, const uint8_t type)
    {
        registers regs{};
        regs.al = type;
        regs.dx = id;
        call(0x0042851C, regs);
    }
}
