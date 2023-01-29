#include "ClimateObject.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

namespace OpenLoco
{
    // 0x00496B1B
    bool ClimateObject::validate() const
    {
        if (winterSnowLine > summerSnowLine)
        {
            return false;
        }
        return firstSeason < 4;
    }

    // 0x00496AF7
    void ClimateObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(ClimateObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Ensure we've loaded the entire object
        assert(remainingData.size() == 0);
    }

    // 0x00496B15
    void ClimateObject::unload()
    {
        name = 0;
    }
}
