#include "TownNamesObject.h"
#include "../Interop/Interop.hpp"
#include <numeric>

namespace OpenLoco
{
    // 0x00498E41
    bool TownNamesObject::validate() const
    {
        const auto total = std::accumulate(
            std::begin(unks), std::end(unks), 1, [](const int32_t res, const Unk& unk) {
                if (unk.count == 0)
                {
                    return res;
                }
                return res * unk.count;
            });
        return total >= kMinNumNameCombinations;
    }

    // 0x00498E1D
    void TownNamesObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00498E1D, regs);
    }

    // 0x00498E3B
    void TownNamesObject::unload()
    {
        name = 0;
    }
}
