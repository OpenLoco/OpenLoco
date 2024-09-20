#include "PaintVehicle.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Paint.h"
#include "Ui/ViewportInteraction.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleDraw.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Math/Trigonometry.hpp>

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

        if (bogie->has38Flags(Flags38::isReversed))
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
                if (sprite.hasFlags(BogieSpriteFlags::rotationalSymmetry))
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
                if (sprite.hasFlags(BogieSpriteFlags::rotationalSymmetry))
                {
                    yawIndex &= 0xF;
                }
                const auto imageIndex = sprite.numFramesPerRotation * yawIndex + bogie->animationIndex + sprite.flatImageIds;
                ImageId imageId{};
                if (bogie->has38Flags(Flags38::isGhost))
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageIndex);
                }
                else if (bogie->hasVehicleFlags(VehicleFlags::unk_5))
                {
                    imageId = ImageId(imageIndex, ExtColour::unk74);
                }
                else if (bogie->getTransportMode() == TransportMode::air)
                {
                    // Airplane bogies are the shadows of the plane

                    if (bogie->has38Flags(Flags38::isGhost))
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

                if (sprite.hasFlags(BogieSpriteFlags::unk_4))
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
                const auto imageIndex = sprite.numFramesPerRotation * yawIndex + bogie->animationIndex + sprite.gentleImageIds;
                ImageId imageId{};
                if (bogie->has38Flags(Flags38::isGhost))
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                    imageId = Gfx::applyGhostToImage(imageIndex);
                }
                else
                {
                    imageId = ImageId(imageIndex, bogie->colourScheme);
                }
                if (sprite.hasFlags(BogieSpriteFlags::unk_4))
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
                const auto imageIndex = sprite.numFramesPerRotation * yawIndex + bogie->animationIndex + sprite.steepImageIds;
                ImageId imageId{};
                if (bogie->has38Flags(Flags38::isGhost))
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

    // 0x004B103C
    static void paintBody(PaintSession& session, VehicleBody* body)
    {
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

        if (body->has38Flags(Flags38::isReversed))
        {
            yaw ^= (1 << 5);
            pitch = kReversePitch[static_cast<uint8_t>(body->spritePitch)];
        }

        uint32_t bodyImageIndex = getBodyImageIndex(sprite, pitch, yaw, body->var_46, body->var_47);

        std::optional<uint32_t> brakingImageIndex = {};
        if (sprite.hasFlags(BodySpriteFlags::hasBrakingLights))
        {
            brakingImageIndex = getBrakingImageIndex(sprite, pitch, yaw);
        }

        World::Pos3 offsets = { 0, 0, body->position.z };
        World::Pos3 boundBoxOffsets;
        World::Pos3 boundBoxSize;
        if ((body->getTransportMode() == TransportMode::air) || (body->getTransportMode() == TransportMode::water))
        {
            boundBoxOffsets = { -8, -8, static_cast<int16_t>(body->position.z + 11) };
            boundBoxSize = { 48, 48, 15 };
        }
        else
        {
            auto& unk = vehObject->var_24[body->bodyIndex];
            auto offsetModifier = unk.length - unk.var_01;
            if (body->has38Flags(Flags38::isReversed))
            {
                offsetModifier = -offsetModifier;
            }

            if (unk.bodySpriteInd & SpriteIndex::flag_unk7)
            {
                offsetModifier = -offsetModifier;
            }

            const auto unk1 = Math::Trigonometry::computeXYVector(offsetModifier, originalYaw) / 2;
            boundBoxOffsets.x = unk1.x;
            boundBoxOffsets.y = unk1.y;
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
        if (body->has38Flags(Flags38::isGhost))
        {
            imageId = Gfx::applyGhostToImage(bodyImageIndex);
        }
        else if (body->hasVehicleFlags(VehicleFlags::unk_5))
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
                && !body->has38Flags(Flags38::isGhost)
                && !body->hasVehicleFlags(VehicleFlags::unk_5))
            {
                session.attachToPrevious(ImageId{ *brakingImageIndex }, { 0, 0 });
            }
        }
    }

    // 0x004B0CCE
    void paintVehicleEntity(PaintSession& session, Vehicles::VehicleBase* base)
    {
        if (base->has38Flags(Flags38::isGhost))
        {
            if (base->owner != CompanyManager::getControllingId())
            {
                return;
            }
        }
        switch (base->getSubType())
        {
            case VehicleEntityType::head:
            case VehicleEntityType::vehicle_1:
            case VehicleEntityType::vehicle_2:
            case VehicleEntityType::tail:
                break;
            case VehicleEntityType::bogie:
                paintBogie(session, base->asVehicleBogie());
                break;
            case VehicleEntityType::body_start:
            case VehicleEntityType::body_continued:
                paintBody(session, base->asVehicleBody());
                break;
        }
    }
}
