#include "VehicleObject.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Utility/Numeric.hpp"
#include "CargoObject.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    void vehicle_object::getCargoString(char* buffer)
    {
        if (num_simultaneous_cargo_types != 0)
        {
            {
                auto cargoType = utility::bitScanForward(primary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = primary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (max_primary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint16_t>(max_primary_cargo);
                        buffer = stringmgr::formatString(buffer, string_ids::stats_capacity, &args);
                    }
                    cargoType = utility::bitScanForward(cargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = utility::bitScanForward(cargoTypes))
                        {
                            cargoTypes &= ~(1 << cargoType);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                            FormatArguments args{};
                            args.push(cargoObj->name);
                            buffer = stringmgr::formatString(buffer, string_ids::stats_or_string, &args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }

            if (flags & flags_E0::refittable)
            {
                buffer = stringmgr::formatString(buffer, string_ids::stats_refittable);
            }

            if (num_simultaneous_cargo_types > 1)
            {
                auto cargoType = utility::bitScanForward(secondary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = secondary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (max_secondary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint16_t>(max_secondary_cargo);
                        buffer = stringmgr::formatString(buffer, string_ids::stats_plus_string, &args);
                    }

                    cargoType = utility::bitScanForward(cargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = utility::bitScanForward(cargoTypes))
                        {
                            cargoTypes &= ~(1 << cargoType);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                            FormatArguments args{};
                            args.push(cargoObj->name);
                            buffer = stringmgr::formatString(buffer, string_ids::stats_or_string, &args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }
        }
    }
}
