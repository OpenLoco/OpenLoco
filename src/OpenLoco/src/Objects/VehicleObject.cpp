#include "VehicleObject.h"
#include "CargoObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "ObjectManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Utility/Numeric.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x004B7733
    static void drawVehicle(Gfx::RenderTarget* rt, const VehicleObject* vehicleObject, uint8_t eax, uint8_t esi, Ui::Point offset)
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
        regs.edi = X86Pointer(rt);
        call(0x4B7733, regs);
    }

    // 0x004B8C52
    void VehicleObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        // Rotation
        uint8_t unk1 = Ui::WindowManager::getVehiclePreviewRotationFrameUnk1();
        uint8_t unk2 = Ui::WindowManager::getVehiclePreviewRotationFrameUnk2();

        drawVehicle(&rt, this, unk1, unk2, Ui::Point{ x, y } + Ui::Point{ 0, 19 });
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static constexpr uint8_t kDescriptionRowHeight = 10;

    // 0x004B8C9D
    void VehicleObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designed, obsolete);
        if (power != 0 && (mode == TransportMode::road || mode == TransportMode::rail))
        {
            FormatArguments args{};
            args.push(power);
            Gfx::drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_power, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push<uint32_t>(StringManager::internalLengthToComma1DP(getLength()));
            Gfx::drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_length, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(weight);
            Gfx::drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_weight, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(speed);
            Gfx::drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_max_speed, &args);
        }
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
        // Clear buffer
        *buffer = '\0';

        getCargoString(buffer);

        if (StringManager::locoStrlen(buffer) != 0)
        {
            Gfx::drawStringLeftWrapped(rt, rowPosition.x, rowPosition.y, width - 4, Colour::black, StringIds::buffer_1250);
        }
    }

    void VehicleObject::getCargoString(char* buffer) const
    {
        if (numSimultaneousCargoTypes != 0)
        {
            {
                auto cargoType = Utility::bitScanForward(primaryCargoTypes);
                if (cargoType != -1)
                {
                    auto cargoTypes = primaryCargoTypes & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unitNamePlural;
                        if (maxPrimaryCargo == 1)
                        {
                            cargoUnitName = cargoObj->unitNameSingular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint32_t>(maxPrimaryCargo);
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

            if (numSimultaneousCargoTypes > 1)
            {
                auto cargoType = Utility::bitScanForward(secondaryCargoTypes);
                if (cargoType != -1)
                {
                    auto cargoTypes = secondaryCargoTypes & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unitNamePlural;
                        if (maxSecondaryCargo == 1)
                        {
                            cargoUnitName = cargoObj->unitNameSingular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint32_t>(maxSecondaryCargo);
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
        if (costIndex > 32)
        {
            return false;
        }
        if (runCostIndex > 32)
        {
            return false;
        }

        if (costFactor <= 0)
        {
            return false;
        }
        if (runCostFactor < 0)
        {
            return false;
        }

        if (flags & FlagsE0::unk_09)
        {
            if (numMods != 0)
            {
                return false;
            }
            if (flags & FlagsE0::rackRail)
            {
                return false;
            }
        }

        if (numMods > 4)
        {
            return false;
        }

        if (numSimultaneousCargoTypes > 2)
        {
            return false;
        }

        if (numCompat > 8)
        {
            return false;
        }

        if (rackSpeed > speed)
        {
            return false;
        }

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

        for (auto& bogieSprite : bogieSprites)
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
    void VehicleObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004B841B, regs);
        if (dependencies != nullptr)
        {
            auto* depObjs = addr<0x0050D158, uint8_t*>();
            dependencies->required.resize(*depObjs++);
            if (!dependencies->required.empty())
            {
                std::copy(reinterpret_cast<ObjectHeader*>(depObjs), reinterpret_cast<ObjectHeader*>(depObjs) + dependencies->required.size(), dependencies->required.data());
                depObjs += sizeof(ObjectHeader) * dependencies->required.size();
            }
            dependencies->willLoad.resize(*depObjs++);
            if (!dependencies->willLoad.empty())
            {
                std::copy(reinterpret_cast<ObjectHeader*>(depObjs), reinterpret_cast<ObjectHeader*>(depObjs) + dependencies->willLoad.size(), dependencies->willLoad.data());
            }
        }
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

        std::fill(std::begin(requiredTrackExtras), std::end(requiredTrackExtras), 0);

        maxPrimaryCargo = 0;
        maxSecondaryCargo = 0;
        primaryCargoTypes = 0;
        secondaryCargoTypes = 0;
        numSimultaneousCargoTypes = 0;

        std::fill(std::begin(cargoTypeSpriteOffsets), std::end(cargoTypeSpriteOffsets), 0);
        std::fill(std::begin(compatibleVehicles), std::end(compatibleVehicles), 0);

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

        for (auto& bogieSprite : bogieSprites)
        {
            bogieSprite.flatImageIds = 0;
            bogieSprite.gentleImageIds = 0;
            bogieSprite.steepImageIds = 0;
            bogieSprite.var_02 = 0;
            bogieSprite.var_03 = 0;
            bogieSprite.var_04 = 0;
            bogieSprite.numRollSprites = 0;
        }

        rackRailType = 0;
        sound.engine1.soundObjectId = 0;

        std::fill(std::begin(startSounds), std::end(startSounds), 0);
    }

    // 0x004B9780
    uint32_t VehicleObject::getLength() const
    {
        auto length = 0;
        for (auto i = 0; i < var_04; ++i)
        {
            if (var_24[i].bodySpriteInd == 0xFF)
            {
                continue;
            }

            auto unk = var_24[i].bodySpriteInd & (VehicleObject::kMaxBodySprites - 1);
            length += bodySprites[unk].bogeyPosition * 2;
        }
        return length;
    }
}
