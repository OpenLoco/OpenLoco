#include "VehicleObject.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Utility/Numeric.hpp"
#include "CargoObject.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    void vehicle_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
    }
    void vehicle_object::getCargoString(char* buffer)
    {
        if (num_simultaneous_cargo_types != 0)
        {
            {
                auto cargoType = Utility::bitScanForward(primary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = primary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (max_primary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint16_t>(max_primary_cargo);
                        buffer = StringManager::formatString(buffer, StringIds::stats_capacity, &args);
                    }
                    cargoType = Utility::bitScanForward(cargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = Utility::bitScanForward(cargoTypes))
                        {
                            cargoTypes &= ~(1 << cargoType);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
                            FormatArguments args{};
                            args.push(cargoObj->name);
                            buffer = StringManager::formatString(buffer, StringIds::stats_or_string, &args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }

            if (flags & FlagsE0::refittable)
            {
                buffer = StringManager::formatString(buffer, StringIds::stats_refittable);
            }

            if (num_simultaneous_cargo_types > 1)
            {
                auto cargoType = Utility::bitScanForward(secondary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = secondary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (max_secondary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint16_t>(max_secondary_cargo);
                        buffer = StringManager::formatString(buffer, StringIds::stats_plus_string, &args);
                    }

                    cargoType = Utility::bitScanForward(cargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = Utility::bitScanForward(cargoTypes))
                        {
                            cargoTypes &= ~(1 << cargoType);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
                            FormatArguments args{};
                            args.push(cargoObj->name);
                            buffer = StringManager::formatString(buffer, StringIds::stats_or_string, &args);
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
