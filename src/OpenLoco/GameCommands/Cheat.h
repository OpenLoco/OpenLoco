#include <cstdint>

namespace OpenLoco::GameCommands
{
    enum class CheatCommand : uint8_t
    {
        acquireAssets,
        addCash,
        clearLoan,
        companyRatings,
        switchCompany,
        toggleBankruptcy,
        toggleJail,
        vehicleReliability,
        modifyDate
    };
}
