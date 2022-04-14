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
    static loco_global<uint16_t, 0x0052622E> _52622E; // Tick related

    // 0x004B7733
    static void drawVehicle(Gfx::Context* context, const VehicleObject* vehicleObject, uint8_t eax, uint8_t esi, Ui::Point offset)
    {
        // Eventually calls 0x4B777B part of 0x4B7741
        registers regs;
        regs.cx = offset.x;
        regs.dx = offset.y;
        regs.eax = eax;
        regs.esi = esi;
        regs.bl = enumValue(Colour::mutedSeaGreen);
        regs.bh = 2;
        regs.ebp = X86Pointer(vehicleObject);
        regs.edi = X86Pointer(context);
        call(0x4B7733, regs);
    }

    // 0x004B8C52
    void VehicleObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        // Rotation
        uint8_t unk1 = _52622E & 0x3F;
        uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;

        drawVehicle(&context, this, unk1, unk2, Ui::Point{ x, y } + Ui::Point{ 0, 19 });
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static const uint8_t descriptionRowHeight = 10;

    // 0x004B8C9D
    void VehicleObject::drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(context, rowPosition, designed, obsolete);
        if (power != 0 && (mode == TransportMode::road || mode == TransportMode::rail))
        {
            FormatArguments args{};
            args.push(power);
            Gfx::drawString_494B3F(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_power, &args);
            rowPosition.y += descriptionRowHeight;
        }

        {
            FormatArguments args{};
            args.push(weight);
            Gfx::drawString_494B3F(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_weight, &args);
            rowPosition.y += descriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(speed);
            Gfx::drawString_494B3F(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_max_speed, &args);
        }
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
        // Clear buffer
        *buffer = '\0';

        getCargoString(buffer);

        if (strlen(buffer) != 0)
        {
            Gfx::drawString_495224(context, rowPosition.x, rowPosition.y, width - 4, Colour::black, StringIds::buffer_1250);
        }
    }

    void VehicleObject::getCargoString(char* buffer) const
    {
        if (num_simultaneous_cargo_types != 0)
        {
            {
                auto cargoType = Utility::bitScanForward(primary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = primary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (max_primary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint32_t>(max_primary_cargo);
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

                            auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
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
                        auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (max_secondary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint32_t>(max_secondary_cargo);
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

                            auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
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
