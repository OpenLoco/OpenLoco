#include "CargoObject.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

namespace OpenLoco
{
    // 0x0042F533
    bool CargoObject::validate() const
    {
        if (var_2 > 3840)
        {
            return false;
        }
        if (var_4 == 0)
        {
            return false;
        }
        return true;
    }

    // 0x0042F4D0
    void CargoObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(CargoObject));

        auto loadString = [&remainingData, &handle](string_id& dst, uint8_t num) {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, num);
            dst = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        };

        // Load object strings
        loadString(name, 0);
        loadString(unitsAndCargoName, 1);
        loadString(unitNameSingular, 2);
        loadString(unitNamePlural, 3);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        unitInlineSprite = imageRes.imageOffset;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x0042F514
    void CargoObject::unload()
    {
        name = 0;
        unitsAndCargoName = 0;
        unitNameSingular = 0;
        unitNamePlural = 0;
        unitInlineSprite = 0;
    }

    CargoObjectFlags CargoObject::getFlags() const
    {
        return flags;
    }

    void CargoObject::setFlags(CargoObjectFlags paramFlags)
    {
        CargoObject::flags = paramFlags;
    }

    bool CargoObject::hasFlags(CargoObjectFlags paramFlags) const
    {
        return (getFlags() & paramFlags) != CargoObjectFlags::none;
    }

    void CargoObject::removeFlags(CargoObjectFlags paramFlags)
    {
        setFlags(getFlags() & ~paramFlags);
    }
}
