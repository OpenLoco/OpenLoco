#include <cstdint>

namespace OpenLoco::GameCommands
{
    enum class CheatCommand : uint8_t
    {
        clearLoan,
        addCash,
        switchCompany,
        vehicleReliability,
    };
}
