#include "VehicleObject.h"
#include "CargoObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
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
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        ObjectManager::drawGenericDescription(rt, rowPosition, designed, obsolete);
        if (power != 0 && (mode == TransportMode::road || mode == TransportMode::rail))
        {
            FormatArguments args{};
            args.push(power);
            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_power, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push<uint32_t>(StringManager::internalLengthToComma1DP(getLength()));
            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_length, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(weight);
            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_weight, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push(speed);
            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_max_speed, &args);
        }
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
        // Clear buffer
        *buffer = '\0';

        getCargoString(buffer);

        if (StringManager::locoStrlen(buffer) != 0)
        {
            drawingCtx.drawStringLeftWrapped(rt, rowPosition.x, rowPosition.y, width - 4, Colour::black, StringIds::buffer_1250);
        }
    }

    void VehicleObject::getCargoString(char* buffer) const
    {
        if (numSimultaneousCargoTypes != 0)
        {
            {
                auto cargoType = Utility::bitScanForward(cargoTypes[0]);
                if (cargoType != -1)
                {
                    auto primaryCargoTypes = cargoTypes[0] & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unitNamePlural;
                        if (maxCargo[0] == 1)
                        {
                            cargoUnitName = cargoObj->unitNameSingular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint32_t>(maxCargo[0]);
                        buffer = StringManager::formatString(buffer, StringIds::stats_capacity, &args);
                    }
                    cargoType = Utility::bitScanForward(primaryCargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = Utility::bitScanForward(primaryCargoTypes))
                        {
                            primaryCargoTypes &= ~(1 << cargoType);
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

            if (hasFlags(VehicleObjectFlags::refittable))
            {
                buffer = StringManager::formatString(buffer, StringIds::stats_refittable);
            }

            if (numSimultaneousCargoTypes > 1)
            {
                auto cargoType = Utility::bitScanForward(cargoTypes[1]);
                if (cargoType != -1)
                {
                    auto secondaryCargoTypes = cargoTypes[1] & ~(1 << cargoType);
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unitNamePlural;
                        if (maxCargo[1] == 1)
                        {
                            cargoUnitName = cargoObj->unitNameSingular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint32_t>(maxCargo[1]);
                        buffer = StringManager::formatString(buffer, StringIds::stats_plus_string, &args);
                    }

                    cargoType = Utility::bitScanForward(secondaryCargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = Utility::bitScanForward(secondaryCargoTypes))
                        {
                            secondaryCargoTypes &= ~(1 << cargoType);
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

        if (hasFlags(VehicleObjectFlags::unk_09))
        {
            if (numMods != 0)
            {
                return false;
            }
            if (hasFlags(VehicleObjectFlags::rackRail))
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
            if (!bodySprite.hasFlags(BodySpriteFlags::hasSprites))
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
            if (!bogieSprite.hasFlags(BogieSpriteFlags::hasSprites))
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

    static constexpr uint8_t getNumRotationSpriteTypesFlat(uint8_t numFrames)
    {
        switch (numFrames)
        {
            case 8:
                return 1;
            case 16:
                return 2;
            case 32:
                return 3;
            default:
                return 4;
        }
    }

    static constexpr uint8_t getNumRotationSpriteTypesSloped(uint8_t numFrames)
    {
        switch (numFrames)
        {
            case 4:
                return 0;
            case 8:
                return 1;
            case 16:
                return 2;
            default:
                return 3;
        }
    }

    // 0x004B841B
    void VehicleObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(VehicleObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        trackType = 0xFF;
        if (!hasFlags(VehicleObjectFlags::unk_09) && (mode == TransportMode::rail || mode == TransportMode::road))
        {
            ObjectHeader trackHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(trackHeader);
            }
            auto res = ObjectManager::findObjectHandle(trackHeader);
            if (res.has_value())
            {
                trackType = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load Extra
        for (auto i = 0U, index = 0U; i < numMods; ++i)
        {
            ObjectHeader modHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(modHeader);
            }
            auto res = ObjectManager::findObjectHandle(modHeader);
            if (res.has_value())
            {
                requiredTrackExtras[index++] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        std::fill(std::begin(cargoTypeSpriteOffsets), std::end(cargoTypeSpriteOffsets), 0);
        std::fill(std::begin(cargoTypes), std::end(cargoTypes), 0);
        numSimultaneousCargoTypes = 0;

        for (auto i = 0U; i < std::size(cargoTypes); ++i)
        {
            const auto index = numSimultaneousCargoTypes;
            maxCargo[index] = *reinterpret_cast<const uint8_t*>(remainingData.data());
            remainingData = remainingData.subspan(sizeof(uint8_t));
            if (maxCargo[index] == 0)
            {
                continue;
            }
            while (*reinterpret_cast<const uint16_t*>(remainingData.data()) != 0xFFFFU)
            {
                const auto cargoMatchFlags = *reinterpret_cast<const uint16_t*>(remainingData.data());
                const auto unk = *reinterpret_cast<const uint8_t*>(remainingData.data());
                remainingData = remainingData.subspan(sizeof(uint16_t) + sizeof(uint8_t));

                for (auto cargoType = 0U; cargoType < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoType)
                {
                    auto* cargoObj = ObjectManager::get<CargoObject>(cargoType);
                    if (cargoObj == nullptr)
                    {
                        continue;
                    }
                    if (cargoObj->matchFlags != cargoMatchFlags)
                    {
                        continue;
                    }
                    cargoTypes[index] |= (1U << cargoType);
                    cargoTypeSpriteOffsets[cargoType] = unk;
                }
            }
            remainingData = remainingData.subspan(sizeof(uint16_t));
            if (cargoTypes[index] == 0)
            {
                maxCargo[index] = 0;
            }
            else
            {
                numSimultaneousCargoTypes++;
            }
        }

        for (auto& anim : animation)
        {
            if (anim.type == simple_animation_type::none)
            {
                continue;
            }
            ObjectHeader modHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(modHeader);
            }
            auto res = ObjectManager::findObjectHandle(modHeader);
            if (res.has_value())
            {
                anim.objectId = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        for (auto i = 0U, index = 0U; i < numCompat; ++i)
        {
            ObjectHeader vehHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandleFuzzy(vehHeader);
            if (res.has_value())
            {
                compatibleVehicles[index++] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        if (hasFlags(VehicleObjectFlags::rackRail))
        {
            ObjectHeader unkHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(unkHeader);
            }
            auto res = ObjectManager::findObjectHandle(unkHeader);
            if (res.has_value())
            {
                rackRailType = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        if (drivingSoundType != DrivingSoundType::none)
        {
            ObjectHeader soundHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(soundHeader);
            }
            auto res = ObjectManager::findObjectHandle(soundHeader);
            if (res.has_value())
            {
                sound.friction.soundObjectId = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        for (auto i = 0; i < (numStartSounds & NumStartSounds::mask); ++i)
        {
            ObjectHeader soundHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(soundHeader);
            }
            auto res = ObjectManager::findObjectHandle(soundHeader);
            if (res.has_value())
            {
                startSounds[i] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        assert(remainingData.size() == imgRes.tableLength);

        auto offset = 0;
        for (auto& bodySprite : bodySprites)
        {
            if (!bodySprite.hasFlags(BodySpriteFlags::hasSprites))
            {
                continue;
            }
            bodySprite.flatImageId = offset + imgRes.imageOffset;
            bodySprite.var_0B = getNumRotationSpriteTypesFlat(bodySprite.numFlatRotationFrames);

            bodySprite.numFramesPerRotation = bodySprite.numAnimationFrames * bodySprite.numCargoFrames * bodySprite.numRollFrames + (bodySprite.hasFlags(BodySpriteFlags::hasBrakingLights) ? 1 : 0);
            const auto numFlatFrames = (bodySprite.numFramesPerRotation * bodySprite.numFlatRotationFrames);
            offset += numFlatFrames / (bodySprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 2 : 1);

            if (bodySprite.hasFlags(BodySpriteFlags::hasGentleSprites))
            {
                bodySprite.gentleImageId = offset + imgRes.imageOffset;
                const auto numGentleFrames = bodySprite.numFramesPerRotation * 8;
                offset += numGentleFrames / (bodySprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 2 : 1);

                bodySprite.var_0C = getNumRotationSpriteTypesSloped(bodySprite.numSlopedRotationFrames);
                const auto numSlopedFrames = bodySprite.numFramesPerRotation * bodySprite.numSlopedRotationFrames * 2;
                offset += numSlopedFrames / (bodySprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 2 : 1);

                if (bodySprite.hasFlags(BodySpriteFlags::hasSteepSprites))
                {
                    bodySprite.steepImageId = offset + imgRes.imageOffset;
                    const auto numSteepFrames = bodySprite.numFramesPerRotation * 8;
                    offset += numSteepFrames / (bodySprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 2 : 1);
                    // TODO: add these two together??
                    const auto numUnkFrames = bodySprite.numSlopedRotationFrames * bodySprite.numFramesPerRotation * 2;
                    offset += numUnkFrames / (bodySprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 2 : 1);
                }
            }

            if (bodySprite.hasFlags(BodySpriteFlags::hasUnkSprites))
            {
                bodySprite.unkImageId = offset + imgRes.imageOffset;
                const auto numUnkFrames = bodySprite.numFlatRotationFrames * 3;
                offset += numUnkFrames / (bodySprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 2 : 1);
            }

            const auto numImages = imgRes.imageOffset + offset - bodySprite.flatImageId;
            const auto extents = Gfx::getImagesMaxExtent(ImageId(bodySprite.flatImageId), numImages);
            bodySprite.var_08 = extents.width;
            bodySprite.var_09 = extents.heightNegative;
            bodySprite.var_0A = extents.heightPositive;
        }

        for (auto& bogieSprite : bogieSprites)
        {
            if (!bogieSprite.hasFlags(BogieSpriteFlags::hasSprites))
            {
                continue;
            }
            bogieSprite.numRollSprites = bogieSprite.rollStates;
            bogieSprite.flatImageIds = offset + imgRes.imageOffset;

            const auto numRollFrames = bogieSprite.numRollSprites * 32;
            offset += numRollFrames / (bogieSprite.hasFlags(BogieSpriteFlags::rotationalSymmetry) ? 2 : 1);

            if (bogieSprite.hasFlags(BogieSpriteFlags::hasGentleSprites))
            {
                bogieSprite.gentleImageIds = offset + imgRes.imageOffset;
                const auto numGentleFrames = bogieSprite.numRollSprites * 64;
                offset += numGentleFrames / (bogieSprite.hasFlags(BogieSpriteFlags::rotationalSymmetry) ? 2 : 1);

                if (bogieSprite.hasFlags(BogieSpriteFlags::hasSteepSprites))
                {
                    bogieSprite.steepImageIds = offset + imgRes.imageOffset;
                    const auto numSteepFrames = bogieSprite.numRollSprites * 64;
                    offset += numSteepFrames / (bogieSprite.hasFlags(BogieSpriteFlags::rotationalSymmetry) ? 2 : 1);
                }
            }

            const auto numImages = imgRes.imageOffset + offset - bogieSprite.flatImageIds;
            const auto extents = Gfx::getImagesMaxExtent(ImageId(bogieSprite.flatImageIds), numImages);
            bogieSprite.var_02 = extents.width;
            bogieSprite.var_03 = extents.heightNegative;
            bogieSprite.var_04 = extents.heightPositive;
        }

        // Verify we haven't overshot any lengths
        assert(imgRes.imageOffset + offset == ObjectManager::getTotalNumImages());
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

        std::fill(std::begin(maxCargo), std::end(maxCargo), 0);
        std::fill(std::begin(cargoTypes), std::end(cargoTypes), 0);
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
