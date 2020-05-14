#include "industry_object.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "cargo_object.h"
#include "objectmgr.h"
#include <algorithm>

using namespace openloco::interop;

namespace openloco
{
    bool industry_object::requiresCargo() const
    {
        auto requiredCargoState = false;
        for (const auto& requiredCargo : required_cargo_type)
        {
            if (requiredCargo != 0xff)
            {
                requiredCargoState = true;
                break;
            }
        }
        return requiredCargoState;
    }

    bool industry_object::producesCargo() const
    {
        auto produceCargoState = false;
        for (const auto& producedCargo : produced_cargo_type)
        {
            if (producedCargo != 0xff)
            {
                produceCargoState = true;
                break;
            }
        }
        return produceCargoState;
    }

    char* industry_object::getProducedCargoString(const char* buffer)
    {
        char* ptr = (char*)buffer;
        auto producedCargoCount = 0;

        for (const auto& producedCargo : produced_cargo_type)
        {
            if (producedCargo != 0xFF)
            {
                producedCargoCount++;

                if (producedCargoCount > 1)
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_and);

                auto cargoObj = objectmgr::get<cargo_object>(producedCargo);
                ptr = stringmgr::format_string(ptr, cargoObj->name);
            }
        }
        return ptr;
    }

    char* industry_object::getRequiredCargoString(const char* buffer)
    {
        char* ptr = (char*)buffer;
        auto requiredCargoCount = 0;

        for (const auto& requiredCargo : required_cargo_type)
        {
            if (requiredCargo != 0xFF)
            {
                requiredCargoCount++;

                if (requiredCargoCount > 1)
                {
                    if ((flags & industry_object_flags::requires_all_cargo) != 0)
                        ptr = stringmgr::format_string(ptr, string_ids::cargo_and);
                    else
                        ptr = stringmgr::format_string(ptr, string_ids::cargo_or);
                }

                auto cargoObj = objectmgr::get<cargo_object>(requiredCargo);
                ptr = stringmgr::format_string(ptr, cargoObj->name);
            }
        }
        return ptr;
    }
}