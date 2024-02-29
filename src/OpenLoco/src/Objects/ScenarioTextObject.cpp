#include "ScenarioTextObject.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

namespace OpenLoco
{
    // 0x0043EDE3
    void ScenarioTextObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(ScenarioTextObject));

        auto loadString = [&remainingData, &handle](StringId& dst, uint8_t num) {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, num);
            dst = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        };

        loadString(name, 0);
        loadString(details, 1);

        assert(remainingData.size() == 0);
    }

    // 0x0043EE0B
    void ScenarioTextObject::unload()
    {
        name = 0;
        details = 0;
    }
}
