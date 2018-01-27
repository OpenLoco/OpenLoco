#include "industry.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco
{
    industry_id_t industry::id() const
    {
        auto first = (industry*)0x005C455C;
        return (industry_id_t)(this - first);
    }

    bool industry::empty() const
    {
        return name == string_ids::null;
    }

    // 0x00453275
    void industry::update()
    {
        registers regs;
        regs.esi = (int32_t)this;
        regs.edx = id();
        call(0x00453275, regs);
    }
}
