#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Ui.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MapToolTip
{
    static loco_global<string_id[20], 0x0050A018> _mapTooltipFormatArguments;
    static loco_global<company_id_t, 0x0050A040> _mapTooltipOwner;

    // 0x004CEEA7
    void open()
    {
        call(0x004CEEA7);
    }

    void setOwner(company_id_t company)
    {
        _mapTooltipOwner = company;
    }

    void reset()
    {
        setOwner(CompanyId::null);
        getArguments().push(StringIds::null);
    }

    FormatArguments getArguments()
    {
        return FormatArguments{ reinterpret_cast<std::byte*>(&*_mapTooltipFormatArguments), 40 };
    }
}
