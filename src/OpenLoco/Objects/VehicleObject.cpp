#include "VehicleObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Numeric.hpp"
#include "CargoObject.h"
#include "ObjectManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
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
        uint8_t unk1 = Ui::WindowManager::getVehiclePreviewRotationFrameUnk1();
        uint8_t unk2 = Ui::WindowManager::getVehiclePreviewRotationFrameUnk2();

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
            Gfx::drawStringLeft(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_power, &args);
            rowPosition.y += descriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push<uint32_t>(StringManager::internalLengthToComma1DP(getLength()));
            Gfx::drawStringLeft(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_length, &args);
            rowPosition.y += descriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(weight);
            Gfx::drawStringLeft(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_weight, &args);
            rowPosition.y += descriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(speed);
            Gfx::drawStringLeft(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_max_speed, &args);
        }
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
        // Clear buffer
        *buffer = '\0';

        getCargoString(buffer);

        if (strlen(buffer) != 0)
        {
            Gfx::drawStringLeftWrapped(context, rowPosition.x, rowPosition.y, width - 4, Colour::black, StringIds::buffer_1250);
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

    // 0x004B8B23
    bool VehicleObject::validate() const
    {
        //if (cost_index > 32)
        //{
        //    return false;
        //}
        //if (run_cost_index > 32)
        //{
        //    return false;
        //}

        if (cost_factor <= 0)
        {
            return false;
        }
        if (run_cost_factor < 0)
        {
            return false;
        }

        if (flags & FlagsE0::unk_09)
        {
            if (num_mods != 0)
            {
                return false;
            }
            if (flags & FlagsE0::rack_rail)
            {
                return false;
            }
        }

        if (num_mods > 4)
        {
            return false;
        }

        if (num_simultaneous_cargo_types > 2)
        {
            return false;
        }

        if (num_compat > 8)
        {
            return false;
        }

        //if (rack_speed > speed)
        //{
        //    return false;
        //}

        for (const auto& bodySprite : bodySprites)
        {
            if (!(bodySprite.flags & BodySpriteFlags::hasSprites))
            {
                continue;
            }

            switch (bodySprite.numFlatRotationFrames)
            {
                case 8:
                case 16:
                case 32:
                case 64:
                case 128:
                    break;
                default:
                    return false;
            }
            switch (bodySprite.numSlopedRotationFrames)
            {
                case 4:
                case 8:
                case 16:
                case 32:
                    break;
                default:
                    return false;
            }
            switch (bodySprite.numAnimationFrames)
            {
                case 1:
                case 2:
                case 4:
                    break;
                default:
                    return false;
            }
            if (bodySprite.numCargoLoadFrames < 1 || bodySprite.numCargoLoadFrames > 5)
            {
                return false;
            }
            switch (bodySprite.numRollFrames)
            {
                case 1:
                case 3:
                    break;
                default:
                    return false;
            }
        }

        for (auto& bogieSprite : bogie_sprites)
        {
            if (!(bogieSprite.flags & BogieSpriteFlags::hasSprites))
            {
                continue;
            }

            switch (bogieSprite.rollStates)
            {
                case 1:
                case 2:
                case 4:
                    break;
                default:
                    return false;
            }
        }
        return true;
    }

    // 0x004B841B
    void VehicleObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004B841B, regs);
    }

    // 0x004B89FF
    void VehicleObject::unload()
    {
        name = 0;
        trackType = 0;
        for (auto& anim : animation)
        {
            anim.objectId = 0;
        }

        std::fill(std::begin(required_track_extras), std::end(required_track_extras), 0);

        max_primary_cargo = 0;
        max_secondary_cargo = 0;
        primary_cargo_types = 0;
        secondary_cargo_types = 0;
        num_simultaneous_cargo_types = 0;

        std::fill(std::begin(cargoTypeSpriteOffsets), std::end(cargoTypeSpriteOffsets), 0);
        std::fill(std::begin(compatible_vehicles), std::end(compatible_vehicles), 0);

        for (auto& bodySprite : bodySprites)
        {
            bodySprite.flatImageId = 0;
            bodySprite.var_0B = 0;
            bodySprite.numFramesPerRotation = 0;
            bodySprite.gentleImageId = 0;
            bodySprite.var_0C = 0;
            bodySprite.steepImageId = 0;
            bodySprite.unkImageId = 0;
            bodySprite.var_08 = 0;
            bodySprite.var_09 = 0;
            bodySprite.var_0A = 0;
        }

        for (auto& bogieSprite : bogie_sprites)
        {
            bogieSprite.flatImageIds = 0;
            bogieSprite.gentleImageIds = 0;
            bogieSprite.steepImageIds = 0;
            bogieSprite.var_02 = 0;
            bogieSprite.var_03 = 0;
            bogieSprite.var_04 = 0;
            bogieSprite.numRollSprites = 0;
        }

        rack_rail_type = 0;
        sound.engine1.soundObjectId = 0;

        std::fill(std::begin(startSounds), std::end(startSounds), 0);
    }

    // 0x004B9780
    uint32_t VehicleObject::getLength() const
    {
        auto length = 0;
        for (auto i = 0; i < var_04; ++i)
        {
            if (var_24[i].body_sprite_ind == 0xFF)
            {
                continue;
            }

            auto unk = var_24[i].body_sprite_ind & (VehicleObject::kMaxBodySprites - 1);
            length += bodySprites[unk].bogey_position * 2;
        }
        return length;
    }
}
