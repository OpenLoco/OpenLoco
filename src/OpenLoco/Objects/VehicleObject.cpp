#include "VehicleObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Utility/Numeric.hpp"
#include "CargoObject.h"
#include "ObjectManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static const xy32 objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };

    static loco_global<uint16_t, 0x0052622E> _52622E; // Tick related

    // 0x004B7733
    static void drawVehicle(Gfx::drawpixelinfo_t* dpi, const vehicle_object* vehicleObject, uint8_t eax, uint8_t esi, Gfx::point_t offset)
    {
        // Eventually calls 0x4B777B part of 0x4B7741
        registers regs;
        regs.cx = offset.x;
        regs.dx = offset.y;
        regs.eax = eax;
        regs.esi = esi;
        regs.bl = Colour::saturated_green;
        regs.bh = 2;
        regs.ebp = (uintptr_t)vehicleObject;
        regs.edi = (uintptr_t)dpi;
        call(0x4B7733, regs);
    }

    // 0x004B8C52
    void vehicle_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;

        xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        if (Gfx::clipDrawpixelinfo(&clipped, &dpi, pos, objectPreviewSize))
        {
            // Rotation
            uint8_t unk1 = _52622E & 0x3F;
            uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;
            drawVehicle(clipped, this, unk1, unk2, { static_cast<int16_t>(objectPreviewOffset.x), 75 });
        }
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
