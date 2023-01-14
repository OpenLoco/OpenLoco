#include "TownNamesObject.h"
#include "Interop/Interop.hpp"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <cassert>
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
    void TownNamesObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(TownNamesObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Ensure we've loaded the entire object
        assert(remainingData.size() == 0);
    }

    // 0x00498E3B
    void TownNamesObject::unload()
    {
        name = 0;
    }
}
