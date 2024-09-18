#include "TownNamesObject.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>
#include <numeric>

namespace OpenLoco
{
    // 0x00498E41
    bool TownNamesObject::validate() const
    {
        const auto total = std::accumulate(
            std::begin(categories), std::end(categories), 1, [](const int32_t res, const Category& category) {
                if (category.count == 0)
                {
                    return res;
                }
                return res * category.count;
            });
        return total >= kMinNumNameCombinations;
    }

    // 0x00498E1D
    void TownNamesObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(TownNamesObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Town name object has an additional structure after this point so can't assert its size
    }

    // 0x00498E3B
    void TownNamesObject::unload()
    {
        name = 0;
    }
}
