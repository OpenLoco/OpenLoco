#include "MessageManager.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "OpenLoco.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::MessageManager
{
    static auto& rawMessages() { return getGameState().messages; }

    Message* get(MessageId id)
    {
        if (enumValue(id) >= Limits::kMaxMessages)
        {
            return nullptr;
        }
        return &rawMessages()[enumValue(id)];
    }

    void post(
        MessageType type,
        CompanyId companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC)
    {
        registers regs;
        regs.al = (uint8_t)type;
        regs.ah = enumValue(companyId);
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
}
