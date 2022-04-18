#include "PaintVehicle.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../Vehicles/Vehicle.h"
#include "Paint.h"

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

        auto& sprite = vehObject->bogie_sprites[bogie->objectSpriteType];
        uint8_t yaw = (bogie->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        auto pitch = bogie->sprite_pitch;

        if (bogie->getFlags38() & Flags38::isReversed)
        {
            // Flip the highest bit to reverse the yaw
            yaw ^= (1 << 5);
            pitch = kReversePitch[static_cast<uint8_t>(bogie->sprite_pitch)];
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
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.flatImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageId);
                }
                else if (bogie->var_0C & Flags0C::unk_5)
                {
                    imageId = Gfx::recolour(imageId, ExtColour::unk74);
                }
                else if (bogie->getTransportMode() == TransportMode::air)
                {
                    // Airplane bogies are the shadows of the plane

                    if (bogie->getFlags38() & Flags38::isGhost)
                    {
                        // Ghosts don't cast shadows
                        return;
                    }
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::recolourTranslucent(imageId, ExtColour::unk32);
                    session.addToPlotList4FD200(imageId, { 0, 0, bogie->position.z }, { 8, 8, static_cast<coord_t>(bogie->position.z + 6) }, { 48, 48, 2 });
                    return;
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colourScheme);
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
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.gentleImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageId);
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colourScheme);
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
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.steepImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageId);
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colourScheme);
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
        uint32_t imageId = (yaw >> _503F20[sprite.var_0B]) * sprite.numFramesPerRotation;
        imageId += sprite.flatImageId;
        return imageId;
    }

    static uint32_t paintBodyPitchUp12Deg(const VehicleObjectBodySprite& sprite, uint8_t yaw)
    {
        if (!(sprite.flags & BodySpriteFlags::hasGentleSprites))
        {
            return paintBodyPitchDefault(sprite, yaw);
        }
        auto imageOffset = sprite.flags & BodySpriteFlags::rotationalSymmetry ? 4 : 8;
        uint32_t imageId = ((yaw >> _503F20[sprite.var_0C]) + imageOffset) * sprite.numFramesPerRotation;
        imageId += sprite.gentleImageId;
        return imageId;
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
        uint32_t imageId = ((yaw >> _503F20[sprite.var_0C]) + imageOffset) * sprite.numFramesPerRotation;
        imageId += sprite.steepImageId;
        return imageId;
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
    static uint32_t getBodyImage(const uint32_t imageId, const VehicleBody* body)
    {
        return imageId + body->var_46 + body->var_47;
    }

    static uint32_t getBrakingImage(const uint32_t imageId, const VehicleObjectBodySprite& sprite)
    {
        // Braking image is the last frame for a rotation
        return imageId + sprite.numFramesPerRotation - 1;
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
        uint8_t yaw = (body->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        auto originalYaw = yaw; // edi
        auto pitch = body->sprite_pitch;

        if (body->getFlags38() & Flags38::isReversed)
        {
            yaw ^= (1 << 5);
            pitch = kReversePitch[static_cast<uint8_t>(body->sprite_pitch)];
        }

        uint32_t pitchImageId;
        switch (pitch)
        {
            case Pitch::flat:
                pitchImageId = paintBodyPitchDefault(sprite, yaw);
                break;
            case Pitch::up12deg:
                pitchImageId = paintBodyPitchUp12Deg(sprite, yaw);
                break;
            case Pitch::down12deg:
                pitchImageId = paintBodyPitchDown12Deg(sprite, yaw);
                break;
            case Pitch::up6deg:
                pitchImageId = paintBodyPitchUp6Deg(sprite, yaw);
                break;
            case Pitch::down6deg:
                pitchImageId = paintBodyPitchDown6Deg(sprite, yaw);
                break;
            case Pitch::up25deg:
                pitchImageId = paintBodyPitchUp25Deg(sprite, yaw);
                break;
            case Pitch::down25deg:
                pitchImageId = paintBodyPitchDown25Deg(sprite, yaw);
                break;
            case Pitch::up18deg:
                pitchImageId = paintBodyPitchUp18Deg(sprite, yaw);
                break;
            case Pitch::down18deg:
                pitchImageId = paintBodyPitchDown18Deg(sprite, yaw);
                break;
            default:
                pitchImageId = paintBodyPitchDefault(sprite, yaw);
                break;
        }

        uint32_t bodyImage = getBodyImage(pitchImageId, body);

        std::optional<uint32_t> brakingImage = {};
        if (sprite.flags & BodySpriteFlags::hasBrakingLights)
        {
            brakingImage = getBrakingImage(pitchImageId, sprite);
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
            if (body->getFlags38() & Flags38::isReversed)
            {
                offsetModifier = -offsetModifier;
            }

            if (unk.body_sprite_ind & SpriteIndex::flag_unk7)
            {
                offsetModifier = -offsetModifier;
            }

            boundBoxOffsets.x = (_503B6A[originalYaw].x * offsetModifier) >> 11;
            boundBoxOffsets.y = (_503B6A[originalYaw].y * offsetModifier) >> 11;
            offsetModifier = sprite.bogey_position * 2 - 4;
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

        uint32_t imageId = 0;
        if (body->getFlags38() & Flags38::isGhost)
        {
            imageId = Gfx::applyGhostToImage(bodyImage);
        }
        else if (body->var_0C & Flags0C::unk_5)
        {
            imageId = Gfx::recolour(bodyImage, ExtColour::unk74);
        }
        else
        {
            imageId = Gfx::recolour2(bodyImage, body->colourScheme);
        }
        session.addToPlotList4FD200(imageId, offsets, boundBoxOffsets, boundBoxSize);

        if (brakingImage)
        {
            Vehicle train(body->head);
            if (train.veh2->var_5B != 0
                && !(body->getFlags38() & Flags38::isGhost)
                && !(body->var_0C & Flags0C::unk_5))
            {
                session.attachToPrevious(*brakingImage, { 0, 0 });
            }
        }
    }

    // 0x004B0CCE
    void paintVehicleEntity(PaintSession& session, Vehicles::VehicleBase* base)
    {
        if (base->getFlags38() & Flags38::isGhost)
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
