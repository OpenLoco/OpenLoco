#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyList
{
    void openPerformanceIndexes()
    {
        call(0x00435C69);
    }

    void openUnk()
    {
        call(0x00435BC8);
    }
}
