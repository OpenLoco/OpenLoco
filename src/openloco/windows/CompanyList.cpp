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

    void OpenUnk()
    {
        call(0x00435BC8);
    }
}
