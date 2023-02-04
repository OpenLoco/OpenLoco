#include "PaintVehicle.h"
#include "CompanyManager.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Paint.h"
#include "Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Vehicles;

namespace OpenLoco::Paint
{
    // 0x00500160
    constexpr Pitch kReversePitch[13]{
        Pitch::flat,
        Pitch::down6deg,
        Pitch::down12deg,
        Pitch::down18deg,
        Pitch::down25deg,
        Pitch::up6deg,
        Pitch::up12deg,
        Pitch::up18deg,
        Pitch::up25deg,
        Pitch::down10deg,
        Pitch::up10deg,
        Pitch::down20deg,
        Pitch::up20deg,
    };

    // 0x00503F20
    const uint8_t _503F20[8]{
        4, 3, 2, 1, 0, 0, 0, 0
    };

    // 0x004B0CFC
    static void paintBogie(PaintSession& session, VehicleBogie* bogie)
    {
        auto* vehObject = ObjectManager::get<VehicleObject>(bogie->objectId);
        if (bogie->objectSpriteType == SpriteIndex::null)
        {
            return;
        }

        auto& sprite = vehObject->bogieSprites[bogie->objectSpriteType];
        uint8_t yaw = (bogie->spriteYaw + (session.getRotation() << 4)) & 0x3F;
        auto pitch = bogie->spritePitch;

        if (bogie->hasFlags(Flags38::isReversed))
        {
            // Flip the highest bit to reverse the yaw
            yaw ^= (1 << 5);
            pitch = kReversePitch[static_cast<uint8_t>(bogie->spritePitch)];
        }
        auto yawIndex = (yaw >> 1) & 0x1F;

        switch (pitch)
        {
            case Pitch::flat:
            case Pitch::up12deg:
            case Pitch::up10deg:
            case Pitch::up25deg:
            case Pitch::up20deg:
                break;
            default:
                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    // Rotational symmetry will have 1 fewer bits of yaw
                    yawIndex ^= (1 << 4);
                }
                else
                {
                    yawIndex ^= (1 << 5);
                }
                break;
        }
        switch (pitch)
        {
            case Pitch::flat:
            {
                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    yawIndex &= 0xF;
                }
                const auto imageIndex = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.flatImageIds;
                ImageId imageId{};
                if (bogie->hasFlags(Flags38::isGhost))
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageIndex);
                }
                else if (bogie->hasFlags(Flags0C::unk_5))
                {
                    imageId = ImageId(imageIndex, ExtColour::unk74);
                }
                else if (bogie->getTransportMode() == TransportMode::air)
                {
                    // Airplane bogies are the shadows of the plane

                    if (bogie->hasFlags(Flags38::isGhost))
                    {
                        // Ghosts don't cast shadows
                        return;
                    }
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = ImageId(imageIndex).withTranslucency(ExtColour::unk32);
                    session.addToPlotList4FD200(imageId, { 0, 0, bogie->position.z }, { 8, 8, static_cast<coord_t>(bogie->position.z + 6) }, { 48, 48, 2 });
                    return;
                }
                else
                {
                    imageId = ImageId(imageIndex, bogie->colourScheme);
                }

                if (sprite.flags & BogieSpriteFlags::unk_4)
                {
                    // larger sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->position.z }, { -9, -9, static_cast<coord_t>(bogie->position.z + 3) }, { 18, 18, 5 });
                }
                else
                {
                    // smaller sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->position.z }, { -6, -6, static_cast<coord_t>(bogie->position.z + 3) }, { 12, 12, 1 });
                }
                break;
            }
            case Pitch::up12deg:
            case Pitch::up10deg:
            case Pitch::down12deg:
            case Pitch::down10deg:
            {
                const auto imageIndex = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.gentleImageIds;
                ImageId imageId{};
                if (bogie->hasFlags(Flags38::isGhost))
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageIndex);
                }
                else
                {
                    imageId = ImageId(imageIndex, bogie->colourScheme);
                }
                if (sprite.flags & BogieSpriteFlags::unk_4)
                {
                    // larger sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->position.z }, { -8, -8, static_cast<coord_t>(bogie->position.z + 3) }, { 16, 16, 1 });
                }
                else
                {
                    // smaller sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->position.z }, { -6, -6, static_cast<coord_t>(bogie->position.z + 3) }, { 12, 12, 1 });
                }
                break;
            }
            default:
            {
                const auto imageIndex = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.steepImageIds;
                ImageId imageId{};
                if (bogie->hasFlags(Flags38::isGhost))
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageIndex);
                }
                else
                {
                    imageId = ImageId(imageIndex, bogie->colourScheme);
                }
                session.addToPlotListAsParent(imageId, { 0, 0, bogie->position.z }, { -6, -6, static_cast<coord_t>(bogie->position.z + 3) }, { 12, 12, 1 });
                break;
            }
        }
    }

    static uint32_t paintBodyPitchDefault(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (sprite.flags & BodySpriteFlags::rotationalSymmetry)
        {
            yaw &= 0x1F;
        }
        uint32_t imageIndex = (yaw >> _503F20[sprite.var_0B]) * sprite.numFramesPerRotation;
        imageIndex += sprite.flatImageId;
        return imageIndex;
    }

    static uint32_t paintBodyPitchUp12Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasGentleSprites))
        {
            return paintBodyPitchDefault(sprite, yaw);
        }
        auto imageOffset = sprite.flags & BodySpriteFlags::rotationalSymmetry ? 4 : 8;
        uint32_t imageIndex = ((yaw >> _503F20[sprite.var_0C]) + imageOffset) * sprite.numFramesPerRotation;
        imageIndex += sprite.gentleImageId;
        return imageIndex;
    }

    static uint32_t paintBodyPitchDown12Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasGentleSprites))
        {
            return paintBodyPitchDefault(sprite, yaw);
        }

        if (sprite.flags & BodySpriteFlags::rotationalSymmetry)
        {
            yaw ^= (1 << 5);
        }
        else
        {
            yaw += (1 << 6);
        }

        return paintBodyPitchUp12Deg(sprite, yaw);
    }

    static uint32_t paintBodyPitchUp6Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasGentleSprites))
        {
            return paintBodyPitchDefault(sprite, yaw);
        }
        yaw += 7;
        yaw >>= 4;
        yaw &= 0x3;

        return yaw * sprite.numFramesPerRotation + sprite.gentleImageId;
    }

    static uint32_t paintBodyPitchDown6Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasGentleSprites))
        {
            return paintBodyPitchDefault(sprite, yaw);
        }
        if (sprite.flags & BodySpriteFlags::rotationalSymmetry)
        {
            yaw ^= (1 << 5);
            return paintBodyPitchUp6Deg(sprite, yaw);
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
    static uint32_t paintBodyPitchUp25Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasSteepSprites))
        {
            return paintBodyPitchUp12Deg(sprite, yaw);
        }
        auto imageOffset = sprite.flags & BodySpriteFlags::rotationalSymmetry ? 4 : 8;
        uint32_t imageIndex = ((yaw >> _503F20[sprite.var_0C]) + imageOffset) * sprite.numFramesPerRotation;
        imageIndex += sprite.steepImageId;
        return imageIndex;
    }

    static uint32_t paintBodyPitchDown25Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasSteepSprites))
        {
            return paintBodyPitchDown12Deg(sprite, yaw);
        }

        if (sprite.flags & BodySpriteFlags::rotationalSymmetry)
        {
            yaw ^= (1 << 5);
        }
        else
        {
            yaw += (1 << 6);
        }

        return paintBodyPitchUp25Deg(sprite, yaw);
    }

    static uint32_t paintBodyPitchUp18Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasSteepSprites))
        {
            return paintBodyPitchUp12Deg(sprite, yaw);
        }
        yaw += 7;
        yaw >>= 4;
        yaw &= 0x3;

        return yaw * sprite.numFramesPerRotation + sprite.steepImageId;
    }

    static uint32_t paintBodyPitchDown18Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasSteepSprites))
        {
            return paintBodyPitchDown12Deg(sprite, yaw);
        }
        if (sprite.flags & BodySpriteFlags::rotationalSymmetry)
        {
            yaw ^= (1 << 5);
            return paintBodyPitchUp18Deg(sprite, yaw);
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

    // Adds roll/animation and cargo
    static uint32_t getBodyImageIndex(const uint32_t imageIndex, const VehicleBody* body)
    {
        return imageIndex + body->var_46 + body->var_47;
    }

    static uint32_t getBrakingImageIndex(const uint32_t imageIndex, const VehicleObjectBodySprite& sprite)
    {
        // Braking image is the last frame for a rotation
        return imageIndex + sprite.numFramesPerRotation - 1;
    }

    // 0x004B103C
    static void paintBody(PaintSession& session, VehicleBody* body)
    {
        static loco_global<Map::Pos2[64], 0x00503B6A> _503B6A;  // also used in vehicle.cpp
        static loco_global<int8_t[32 * 4], 0x005001B4> _5001B4; // array of 4 byte structures

        auto* vehObject = ObjectManager::get<VehicleObject>(body->objectId);
        if (body->objectSpriteType == SpriteIndex::null)
        {
            return;
        }

        auto& sprite = vehObject->bodySprites[body->objectSpriteType];
        uint8_t yaw = (body->spriteYaw + (session.getRotation() << 4)) & 0x3F;
        auto originalYaw = yaw; // edi
        auto pitch = body->spritePitch;

        if (body->hasFlags(Flags38::isReversed))
        {
            yaw ^= (1 << 5);
            pitch = kReversePitch[static_cast<uint8_t>(body->spritePitch)];
        }

        uint32_t pitchImageIndex;
        switch (pitch)
        {
            case Pitch::flat:
                pitchImageIndex = paintBodyPitchDefault(sprite, yaw);
                break;
            case Pitch::up12deg:
                pitchImageIndex = paintBodyPitchUp12Deg(sprite, yaw);
                break;
            case Pitch::down12deg:
                pitchImageIndex = paintBodyPitchDown12Deg(sprite, yaw);
                break;
            case Pitch::up6deg:
                pitchImageIndex = paintBodyPitchUp6Deg(sprite, yaw);
                break;
            case Pitch::down6deg:
                pitchImageIndex = paintBodyPitchDown6Deg(sprite, yaw);
                break;
            case Pitch::up25deg:
                pitchImageIndex = paintBodyPitchUp25Deg(sprite, yaw);
                break;
            case Pitch::down25deg:
                pitchImageIndex = paintBodyPitchDown25Deg(sprite, yaw);
                break;
            case Pitch::up18deg:
                pitchImageIndex = paintBodyPitchUp18Deg(sprite, yaw);
                break;
            case Pitch::down18deg:
                pitchImageIndex = paintBodyPitchDown18Deg(sprite, yaw);
                break;
            default:
                pitchImageIndex = paintBodyPitchDefault(sprite, yaw);
                break;
        }

        uint32_t bodyImageIndex = getBodyImageIndex(pitchImageIndex, body);

        std::optional<uint32_t> brakingImageIndex = {};
        if (sprite.flags & BodySpriteFlags::hasBrakingLights)
        {
            brakingImageIndex = getBrakingImageIndex(pitchImageIndex, sprite);
        }

        Map::Pos3 offsets = { 0, 0, body->position.z };
        Map::Pos3 boundBoxOffsets;
        Map::Pos3 boundBoxSize;
        if ((body->getTransportMode() == TransportMode::air) || (body->getTransportMode() == TransportMode::water))
        {
            boundBoxOffsets = { -8, -8, static_cast<int16_t>(body->position.z + 11) };
            boundBoxSize = { 48, 48, 15 };
        }
        else
        {
            auto& unk = vehObject->var_24[body->bodyIndex];
            auto offsetModifier = unk.length - unk.var_01;
            if (body->hasFlags(Flags38::isReversed))
            {
                offsetModifier = -offsetModifier;
            }

            if (unk.bodySpriteInd & SpriteIndex::flag_unk7)
            {
                offsetModifier = -offsetModifier;
            }

            boundBoxOffsets.x = (_503B6A[originalYaw].x * offsetModifier) >> 11;
            boundBoxOffsets.y = (_503B6A[originalYaw].y * offsetModifier) >> 11;
            offsetModifier = sprite.bogeyPosition * 2 - 4;
            originalYaw &= 0x1F;
            boundBoxOffsets.x += (_5001B4[originalYaw * 4] * offsetModifier) >> 8;
            boundBoxOffsets.y += (_5001B4[originalYaw * 4 + 1] * offsetModifier) >> 8;
            boundBoxOffsets.z = body->position.z + 11;
            boundBoxSize = {
                static_cast<coord_t>((_5001B4[originalYaw * 4 + 2] * offsetModifier) >> 8),
                static_cast<coord_t>((_5001B4[originalYaw * 4 + 3] * offsetModifier) >> 8),
                15
            };
        }

        ImageId imageId{};
        if (body->hasFlags(Flags38::isGhost))
        {
            imageId = Gfx::applyGhostToImage(bodyImageIndex);
        }
        else if (body->hasFlags(Flags0C::unk_5))
        {
            imageId = ImageId(bodyImageIndex, ExtColour::unk74);
        }
        else
        {
            imageId = ImageId(bodyImageIndex, body->colourScheme);
        }
        session.addToPlotList4FD200(imageId, offsets, boundBoxOffsets, boundBoxSize);

        if (brakingImageIndex)
        {
            Vehicle train(body->head);
            if (train.veh2->var_5B != 0
                && !body->hasFlags(Flags38::isGhost)
                && !body->hasFlags(Flags0C::unk_5))
            {
                session.attachToPrevious(ImageId{ *brakingImageIndex }, { 0, 0 });
            }
        }
    }

    // 0x004B0CCE
    void paintVehicleEntity(PaintSession& session, Vehicles::VehicleBase* base)
    {
        if (base->hasFlags(Flags38::isGhost))
        {
            if (base->owner != CompanyManager::getControllingId())
            {
                return;
            }
        }
        switch (base->getSubType())
        {
            case VehicleThingType::head:
            case VehicleThingType::vehicle_1:
            case VehicleThingType::vehicle_2:
            case VehicleThingType::tail:
                break;
            case VehicleThingType::bogie:
                paintBogie(session, base->asVehicleBogie());
                break;
            case VehicleThingType::body_start:
            case VehicleThingType::body_continued:
                paintBody(session, base->asVehicleBody());
                break;
        }
    }
}
