#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyFaceSelection
{
    void open(company_id_t id)
    {
        registers regs;
        regs.eax = id;
        call(0x00434F52, regs);
    }
}
