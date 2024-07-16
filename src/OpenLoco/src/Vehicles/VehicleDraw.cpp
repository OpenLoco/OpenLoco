#include "VehicleDraw.h"
#include "Entities/Entity.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageId.h"
#include "Graphics/SoftwareDrawingContext.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Map/Tile.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/VehicleObject.h"
#include "Vehicle.h"
#include "Viewport.hpp"
#include "World/CompanyManager.h"
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <OpenLoco/Math/Trigonometry.hpp>
#include <array>
#include <sfl/static_vector.hpp>

namespace OpenLoco
{
    // 0x00503F20
    static constexpr std::array<uint8_t, 8> _503F20{
        4, 3, 2, 1, 0, 0, 0, 0
    };

    static uint32_t getBodyImageIndexPitchDefault(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (sprite.hasFlags(BodySpriteFlags::rotationalSymmetry))
        {
            yaw &= 0x1F;
        }
        uint32_t imageIndex = (yaw >> _503F20[sprite.flatYawAccuracy]) * sprite.numFramesPerRotation;
        imageIndex += sprite.flatImageId;
        return imageIndex;
    }

    static uint32_t getBodyImageIndexPitchUp12Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasGentleSprites))
        {
            return getBodyImageIndexPitchDefault(sprite, yaw);
        }
        auto imageOffset = sprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 4 : 8;
        uint32_t imageIndex = ((yaw >> _503F20[sprite.slopedYawAccuracy]) + imageOffset) * sprite.numFramesPerRotation;
        imageIndex += sprite.gentleImageId;
        return imageIndex;
    }

    static uint32_t getBodyImageIndexPitchDown12Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasGentleSprites))
        {
            return getBodyImageIndexPitchDefault(sprite, yaw);
        }

        if (sprite.hasFlags(BodySpriteFlags::rotationalSymmetry))
        {
            yaw ^= (1 << 5);
        }
        else
        {
            yaw += (1 << 6);
        }

        return getBodyImageIndexPitchUp12Deg(sprite, yaw);
    }

    static uint32_t getBodyImageIndexPitchUp6Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasGentleSprites))
        {
            return getBodyImageIndexPitchDefault(sprite, yaw);
        }
        yaw += 7;
        yaw >>= 4;
        yaw &= 0x3;

        return yaw * sprite.numFramesPerRotation + sprite.gentleImageId;
    }

    static uint32_t getBodyImageIndexPitchDown6Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasGentleSprites))
        {
            return getBodyImageIndexPitchDefault(sprite, yaw);
        }
        if (sprite.hasFlags(BodySpriteFlags::rotationalSymmetry))
        {
            yaw ^= (1 << 5);
            return getBodyImageIndexPitchUp6Deg(sprite, yaw);
        }
        else
        {
            yaw += 7;
            yaw >>= 4;
            yaw &= 0x3;
            yaw += 4;

            return yaw * sprite.numFramesPerRotation + sprite.gentleImageId;
        }
    }
    static uint32_t getBodyImageIndexPitchUp25Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasSteepSprites))
        {
            return getBodyImageIndexPitchUp12Deg(sprite, yaw);
        }
        auto imageOffset = sprite.hasFlags(BodySpriteFlags::rotationalSymmetry) ? 4 : 8;
        uint32_t imageIndex = ((yaw >> _503F20[sprite.slopedYawAccuracy]) + imageOffset) * sprite.numFramesPerRotation;
        imageIndex += sprite.steepImageId;
        return imageIndex;
    }

    static uint32_t getBodyImageIndexPitchDown25Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasSteepSprites))
        {
            return getBodyImageIndexPitchDown12Deg(sprite, yaw);
        }

        if (sprite.hasFlags(BodySpriteFlags::rotationalSymmetry))
        {
            yaw ^= (1 << 5);
        }
        else
        {
            yaw += (1 << 6);
        }

        return getBodyImageIndexPitchUp25Deg(sprite, yaw);
    }

    static uint32_t getBodyImageIndexPitchUp18Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasSteepSprites))
        {
            return getBodyImageIndexPitchUp12Deg(sprite, yaw);
        }
        yaw += 7;
        yaw >>= 4;
        yaw &= 0x3;

        return yaw * sprite.numFramesPerRotation + sprite.steepImageId;
    }

    static uint32_t getBodyImageIndexPitchDown18Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!sprite.hasFlags(BodySpriteFlags::hasSteepSprites))
        {
            return getBodyImageIndexPitchDown12Deg(sprite, yaw);
        }
        if (sprite.hasFlags(BodySpriteFlags::rotationalSymmetry))
        {
            yaw ^= (1 << 5);
            return getBodyImageIndexPitchUp18Deg(sprite, yaw);
        }
        else
        {
            yaw += 7;
            yaw >>= 4;
            yaw &= 0x3;
            yaw += 4;

            return yaw * sprite.numFramesPerRotation + sprite.steepImageId;
        }
    }

    static uint32_t getPitchBodyImageIndex(const VehicleObjectBodySprite& sprite, const Pitch pitch, const uint8_t yaw)
    {
        uint32_t pitchImageIndex;
        switch (pitch)
        {
            case Pitch::flat:
                pitchImageIndex = getBodyImageIndexPitchDefault(sprite, yaw);
                break;
            case Pitch::up12deg:
                pitchImageIndex = getBodyImageIndexPitchUp12Deg(sprite, yaw);
                break;
            case Pitch::down12deg:
                pitchImageIndex = getBodyImageIndexPitchDown12Deg(sprite, yaw);
                break;
            case Pitch::up6deg:
                pitchImageIndex = getBodyImageIndexPitchUp6Deg(sprite, yaw);
                break;
            case Pitch::down6deg:
                pitchImageIndex = getBodyImageIndexPitchDown6Deg(sprite, yaw);
                break;
            case Pitch::up25deg:
                pitchImageIndex = getBodyImageIndexPitchUp25Deg(sprite, yaw);
                break;
            case Pitch::down25deg:
                pitchImageIndex = getBodyImageIndexPitchDown25Deg(sprite, yaw);
                break;
            case Pitch::up18deg:
                pitchImageIndex = getBodyImageIndexPitchUp18Deg(sprite, yaw);
                break;
            case Pitch::down18deg:
                pitchImageIndex = getBodyImageIndexPitchDown18Deg(sprite, yaw);
                break;
            default:
                pitchImageIndex = getBodyImageIndexPitchDefault(sprite, yaw);
                break;
        }
        return pitchImageIndex;
    }

    uint32_t getBodyImageIndex(const VehicleObjectBodySprite& sprite, const Pitch pitch, const uint8_t yaw, const uint8_t roll, const uint8_t cargoIndex)
    {
        const auto pitchImageIndex = getPitchBodyImageIndex(sprite, pitch, yaw);
        return pitchImageIndex + roll + cargoIndex;
    }
    uint32_t getBrakingImageIndex(const VehicleObjectBodySprite& sprite, const Pitch pitch, const uint8_t yaw)
    {
        const auto pitchImageIndex = getPitchBodyImageIndex(sprite, pitch, yaw);
        // Braking image is the last frame for a rotation
        return pitchImageIndex + sprite.numFramesPerRotation - 1;
    }

    constexpr std::array<uint8_t, 8> kUnk500264 = {
        0,
        1,
        1,
        1,
        0,
        2,
        2,
        2,
    };

    struct DrawItem
    {
        ImageId image;
        int32_t dist;
        bool isBody;
    };

    static void drawItem(Gfx::DrawingContext& drawingCtx, Ui::Point offset, const DrawItem& item, int32_t midDist, uint8_t yaw)
    {
        const auto factor = midDist - item.dist;
        const auto unk1 = Math::Trigonometry::computeXYVector(factor, yaw) / 4;

        const auto p1 = World::gameToScreen(World::Pos3(unk1.x, unk1.y, 0), 0);

        drawingCtx.drawImage(offset + Ui::Point(p1.x, p1.y), item.image);
    }

    struct DrawItems
    {
        // Max num items is kMaxBodySprites * (2 bogies + 1 body)
        sfl::static_vector<DrawItem, VehicleObject::kMaxBodySprites*(2 + 1)> items;
        int32_t totalDistance;
    };

    static DrawItems getDrawItemsForVehicle(const VehicleObject& vehObject, const uint8_t yaw, const uint8_t roll, const ColourScheme colourScheme)
    {
        DrawItems drawItems{};
        const auto unk11360E8 = kUnk500264[roll & 0x7];
        for (auto i = 0; i < vehObject.var_04; ++i)
        {
            if (vehObject.hasFlags(VehicleObjectFlags::flag_02) && i == 0)
            {
                continue;
            }
            if (vehObject.hasFlags(VehicleObjectFlags::flag_03) && i + 1 == vehObject.var_04)
            {
                continue;
            }

            auto& v24 = vehObject.var_24[i];
            // 0x01136172
            auto unkDist = v24.length;

            if (v24.frontBogieSpriteInd != SpriteIndex::null && (vehObject.mode == TransportMode::rail || vehObject.mode == TransportMode::road))
            {
                auto unk = yaw;
                if (v24.bodySpriteInd & SpriteIndex::flag_unk7)
                {
                    unk ^= 1U << 5;
                }
                unk /= 2;

                auto& bogieSprites = vehObject.bogieSprites[v24.frontBogieSpriteInd];
                if (bogieSprites.hasFlags(BogieSpriteFlags::rotationalSymmetry))
                {
                    unk &= 0xFU;
                }

                const auto rollIndex = (bogieSprites.numRollSprites - 1) & roll;
                auto spriteIndex = bogieSprites.numRollSprites * unk + rollIndex + bogieSprites.flatImageIds;
                drawItems.items.push_back(DrawItem{ ImageId(spriteIndex, colourScheme), drawItems.totalDistance + unkDist, false });
            }

            auto unk1136170 = 0;
            auto backBogieDist = drawItems.totalDistance;
            if (v24.bodySpriteInd != SpriteIndex::null)
            {
                auto& bodySprites = vehObject.bodySprites[v24.bodySpriteInd & ~(1U << 7)];
                unk1136170 = bodySprites.bogeyPosition * 2;
                backBogieDist += unk1136170;
            }
            auto unk1136174 = v24.var_01;
            backBogieDist -= v24.var_01;

            if (v24.backBogieSpriteInd != SpriteIndex::null && (vehObject.mode == TransportMode::rail || vehObject.mode == TransportMode::road))
            {
                auto unk = yaw;
                if (!(v24.bodySpriteInd & SpriteIndex::flag_unk7))
                {
                    unk ^= 1U << 5;
                }
                unk /= 2;

                auto& bogieSprites = vehObject.bogieSprites[v24.backBogieSpriteInd];
                if (bogieSprites.hasFlags(BogieSpriteFlags::rotationalSymmetry))
                {
                    unk &= 0xFU;
                }

                const auto rollIndex = (bogieSprites.numRollSprites - 1) & roll;
                auto spriteIndex = bogieSprites.numRollSprites * unk + rollIndex + bogieSprites.flatImageIds;
                drawItems.items.push_back(DrawItem{ ImageId(spriteIndex, colourScheme), backBogieDist, false });
            }

            auto bodyDist = drawItems.totalDistance + (unkDist + unk1136170 - unk1136174) / 2;
            if (v24.bodySpriteInd != SpriteIndex::null)
            {
                auto& bodySprites = vehObject.bodySprites[v24.bodySpriteInd & ~SpriteIndex::flag_unk7];

                auto unk = yaw;
                if (v24.bodySpriteInd & SpriteIndex::flag_unk7)
                {
                    unk ^= 1U << 5;
                }

                uint8_t rollIndex = 0;
                if (!bodySprites.hasFlags(BodySpriteFlags::hasSpeedAnimation))
                {
                    rollIndex = (bodySprites.numAnimationFrames - 1) & roll;
                }
                if (bodySprites.numRollFrames != 1)
                {
                    rollIndex += unk11360E8;
                }

                auto spriteIndex = getBodyImageIndex(bodySprites, Pitch::flat, unk, rollIndex, 0);

                drawItems.items.push_back(DrawItem{ ImageId(spriteIndex, colourScheme), bodyDist, true });
            }
            drawItems.totalDistance += unk1136170;
        }
        return drawItems;
    }

    static DrawItems getDrawItemsForVehicle(const VehicleObject& vehObject, const uint8_t yaw, const Vehicles::Car& car, const VehicleInlineMode mode)
    {
        Vehicles::Vehicle train(car.front->head);
        DrawItems drawItems{};
        const auto isReversed = car.body->has38Flags(Vehicles::Flags38::isReversed);
        const auto isAnimated = mode == VehicleInlineMode::animated;
        uint8_t componentIndex = isReversed ? vehObject.var_04 - 1 : 0;
        for (auto& carComponent : car)
        {
            auto& v24 = vehObject.var_24[componentIndex];
            // 0x01136172
            auto unkDist = isReversed ? v24.var_01 : v24.length;

            if (carComponent.front->objectSpriteType != SpriteIndex::null && (vehObject.mode == TransportMode::rail || vehObject.mode == TransportMode::road))
            {
                auto unk = yaw;
                if (isReversed)
                {
                    unk ^= 1U << 5;
                }
                unk /= 2;

                auto& bogieSprites = vehObject.bogieSprites[carComponent.front->objectSpriteType];
                if (bogieSprites.hasFlags(BogieSpriteFlags::rotationalSymmetry))
                {
                    unk &= 0xFU;
                }

                const auto rollIndex = isAnimated ? carComponent.front->var_46 : 0;
                auto spriteIndex = bogieSprites.numRollSprites * unk + rollIndex + bogieSprites.flatImageIds;
                drawItems.items.push_back(DrawItem{ ImageId(spriteIndex, carComponent.front->colourScheme), drawItems.totalDistance + unkDist, false });
            }

            auto unk1136170 = 0;
            auto backBogieDist = drawItems.totalDistance;
            if (v24.bodySpriteInd != SpriteIndex::null)
            {
                auto& bodySprites = vehObject.bodySprites[v24.bodySpriteInd & ~(1U << 7)];
                unk1136170 = bodySprites.bogeyPosition * 2;
                backBogieDist += unk1136170;
            }
            auto unk1136174 = isReversed ? v24.length : v24.var_01;
            backBogieDist -= unk1136174;

            if (carComponent.back->objectSpriteType != SpriteIndex::null && (vehObject.mode == TransportMode::rail || vehObject.mode == TransportMode::road))
            {
                auto unk = yaw;
                if (!isReversed)
                {
                    unk ^= 1U << 5;
                }
                unk /= 2;

                auto& bogieSprites = vehObject.bogieSprites[carComponent.back->objectSpriteType];
                if (bogieSprites.hasFlags(BogieSpriteFlags::rotationalSymmetry))
                {
                    unk &= 0xFU;
                }

                const auto rollIndex = isAnimated ? carComponent.back->var_46 : 0;
                auto spriteIndex = bogieSprites.numRollSprites * unk + rollIndex + bogieSprites.flatImageIds;
                drawItems.items.push_back(DrawItem{ ImageId(spriteIndex, carComponent.back->colourScheme), backBogieDist, false });
            }

            auto bodyDist = drawItems.totalDistance + (unkDist + unk1136170 - unk1136174) / 2;
            if (carComponent.body->objectSpriteType != SpriteIndex::null)
            {
                auto& bodySprites = vehObject.bodySprites[carComponent.body->objectSpriteType];

                auto unk = yaw;
                if (isReversed)
                {
                    unk ^= 1U << 5;
                }

                auto rollIndex = isAnimated ? carComponent.body->var_46 : 0;
                rollIndex += carComponent.body->var_47;

                auto spriteIndex = getBodyImageIndex(bodySprites, Pitch::flat, unk, rollIndex, 0);
                drawItems.items.push_back(DrawItem{ ImageId(spriteIndex, carComponent.body->colourScheme), bodyDist, true });
                if (isAnimated
                    && bodySprites.hasFlags(BodySpriteFlags::hasBrakingLights)
                    && train.veh2->var_5B != 0)
                {
                    const auto brakingImageIndex = getBrakingImageIndex(bodySprites, Pitch::flat, yaw);
                    drawItems.items.push_back(DrawItem{ ImageId(brakingImageIndex, carComponent.body->colourScheme), bodyDist, true });
                }
            }
            drawItems.totalDistance += unk1136170;
            if (isReversed)
            {
                componentIndex--;
            }
            else
            {
                componentIndex++;
            }
        }
        return drawItems;
    }

    static int32_t getVehicleObjectYDisplayOffset(const VehicleObject& vehObject)
    {
        auto trackType = vehObject.trackType;
        if (vehObject.mode == TransportMode::road)
        {
            if (trackType != 0xFFU)
            {
                return ObjectManager::get<RoadObject>(trackType)->displayOffset;
            }
        }
        else
        {
            if (trackType == 0xFFU)
            {
                return 3;
            }
            else
            {
                return ObjectManager::get<TrackObject>(trackType)->displayOffset;
            }
        }
        return 0;
    }

    // 0x004B777B
    void drawVehicleOverview(Gfx::DrawingContext& drawingCtx, Ui::Point offset, const VehicleObject& vehObject, const uint8_t yaw, const uint8_t roll, const ColourScheme colourScheme)
    {
        offset.y += getVehicleObjectYDisplayOffset(vehObject);

        // 0x01136152 stores offset.x
        // 0x01136154 stores offset.y

        const auto drawItems = getDrawItemsForVehicle(vehObject, yaw, roll, colourScheme);

        const auto midDist = drawItems.totalDistance / 2;

        // First draw all the bogies
        for (auto& item : drawItems.items)
        {
            if (!item.isBody)
            {
                drawItem(drawingCtx, offset, item, midDist, yaw);
            }
        }

        // Then draw the bodies forward/backward depending on where we are in the rotation
        if (yaw >= 8 && yaw < 40)
        {
            for (auto iter = drawItems.items.rbegin(); iter != drawItems.items.rend(); ++iter)
            {
                auto& item = *iter;
                if (item.isBody)
                {
                    drawItem(drawingCtx, offset, item, midDist, yaw);
                }
            }
        }
        else
        {
            for (auto& item : drawItems.items)
            {
                if (item.isBody)
                {
                    drawItem(drawingCtx, offset, item, midDist, yaw);
                }
            }
        }
    }

    static ColourScheme getCompanyVehicleColourScheme(CompanyId companyId, const VehicleObject& vehObject)
    {
        auto* company = CompanyManager::get(companyId);

        auto colourScheme = company->mainColours;
        if (company->customVehicleColoursSet & (1 << vehObject.colourType))
        {
            colourScheme = company->vehicleColours[vehObject.colourType - 1];
        }
        return colourScheme;
    }

    // 0x004B7741
    void drawVehicleOverview(Gfx::DrawingContext& drawingCtx, Ui::Point offset, int16_t vehicleTypeIdx, uint8_t yaw, uint8_t roll, CompanyId companyId)
    {
        auto* vehObject = ObjectManager::get<VehicleObject>(vehicleTypeIdx);

        auto colourScheme = getCompanyVehicleColourScheme(companyId, *vehObject);

        drawVehicleOverview(drawingCtx, offset, *vehObject, yaw, roll, colourScheme);
    }

    static DrawItems toScreenDistDrawItems(const DrawItems& drawItems, const uint8_t yaw)
    {
        DrawItems screenDistDrawItems{};

        const auto getScreenDistance = [](int32_t gameDist, uint8_t yaw) {
            const auto unk1 = Math::Trigonometry::computeXYVector(gameDist, yaw);

            const auto p1 = World::gameToScreen(World::Pos3(unk1.x, unk1.y, 0), 0);
            // This /4 is meant to be after the compute but we need to carry it to here
            // to keep precision high
            return -p1.x / 4;
        };

        screenDistDrawItems.totalDistance = getScreenDistance(drawItems.totalDistance, yaw);

        for (auto& item : drawItems.items)
        {
            screenDistDrawItems.items.push_back(DrawItem{ item.image, getScreenDistance(item.dist, yaw), item.isBody });
        }
        return screenDistDrawItems;
    }

    // 0x004B7711 & 0x004B745B
    int16_t drawVehicleInline(Gfx::DrawingContext& drawingCtx, int16_t vehicleTypeIdx, CompanyId company, Ui::Point loc)
    {
        // This has been simplified from vanilla.

        loc.y += 19;
        auto* vehObject = ObjectManager::get<VehicleObject>(vehicleTypeIdx);

        loc.y += getVehicleObjectYDisplayOffset(*vehObject);

        auto colourScheme = getCompanyVehicleColourScheme(company, *vehObject);

        const auto yaw = 40;

        const auto drawItems = getDrawItemsForVehicle(*vehObject, yaw, 0, colourScheme);

        auto screenDistDrawItems = toScreenDistDrawItems(drawItems, yaw);

        for (auto& item : screenDistDrawItems.items)
        {
            drawingCtx.drawImage(loc + Ui::Point(item.dist, 0), item.image);
        }
        return screenDistDrawItems.totalDistance;
    }

    // 0x004B6D93
    int16_t drawVehicleInline(Gfx::DrawingContext& drawingCtx, const Vehicles::Car& car, Ui::Point loc, VehicleInlineMode mode, std::optional<Colour> disabled)
    {
        // This has been simplified from vanilla.

        loc.y += 19;
        auto* vehObject = ObjectManager::get<VehicleObject>(car.front->objectId);

        loc.y += getVehicleObjectYDisplayOffset(*vehObject);

        const auto yaw = 40;

        const auto drawItems = getDrawItemsForVehicle(*vehObject, yaw, car, mode);

        auto screenDistDrawItems = toScreenDistDrawItems(drawItems, yaw);

        for (auto& item : screenDistDrawItems.items)
        {
            if (disabled.has_value())
            {
                const auto shade1 = Colours::getShade(disabled.value(), 5);
                const auto shade2 = Colours::getShade(disabled.value(), 3);
                drawingCtx.drawImageSolid(loc + Ui::Point(item.dist, 0) + Ui::Point(1, 1), item.image, shade1);
                drawingCtx.drawImageSolid(loc + Ui::Point(item.dist, 0), item.image, shade2);
            }
            else
            {
                drawingCtx.drawImage(loc + Ui::Point(item.dist, 0), item.image);
            }
        }
        return screenDistDrawItems.totalDistance;
    }

    int16_t getWidthVehicleInline(const Vehicles::Car& car)
    {
        auto* vehObject = ObjectManager::get<VehicleObject>(car.front->objectId);

        const auto yaw = 40;

        const auto drawItems = getDrawItemsForVehicle(*vehObject, yaw, car, VehicleInlineMode::basic);

        auto screenDistDrawItems = toScreenDistDrawItems(drawItems, yaw);

        return screenDistDrawItems.totalDistance;
    }

    // 0x004B6D43
    int16_t drawTrainInline(Gfx::DrawingContext& drawingCtx, const Vehicles::Vehicle& train, Ui::Point loc)
    {
        // Vanilla does this slightly differently draw bogie then draw body
        for (auto& car : train.cars)
        {
            loc.x += drawVehicleInline(drawingCtx, car, loc, VehicleInlineMode::animated);
        }
        return loc.x;
    }
}
