#include "VehicleCrashEffect.h"
#include "Audio/Audio.h"
#include "Entities/EntityManager.h"
#include "Map/TileManager.h"
#include "Random.h"
#include "SplashEffect.h"

#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // There are a total of 12 sprites for this effect, the paint code divides this by 256.
    static constexpr uint16_t kMaxSplashFrames = 12 * 256;

    // Advances the frame counter by 85 each tick.
    static constexpr uint16_t kAnimationSpeed = 85;

    // Unknown which unit this is in, but it is used to calculate the acceleration for gravity.
    static constexpr int32_t kGravity = 5041;

    // 0x004406A0
    void VehicleCrashParticle::update()
    {
        invalidateSprite();

        timeToLive--;
        if (timeToLive == 0)
        {
            EntityManager::freeEntity(this);
            return;
        }

        // Apply gravity
        accelerationZ -= kGravity;

        // Apply air resistance
        accelerationX -= (accelerationX / 256);
        accelerationY -= (accelerationY / 256);
        accelerationZ -= (accelerationZ / 256);

        // Update velocity and position
        int32_t vx = velocity.x + accelerationX;
        int32_t vy = velocity.y + accelerationY;
        int32_t vz = velocity.z + accelerationZ;

        auto newLoc = position + World::Pos3{ static_cast<coord_t>(vx >> 16), static_cast<coord_t>(vy >> 16), static_cast<coord_t>(vz >> 16) };

        velocity = World::Pos3{ static_cast<coord_t>(vx & 0xFFFF), static_cast<coord_t>(vy & 0xFFFF), static_cast<coord_t>(vz & 0xFFFF) };

        // Check collision with land / water
        const auto tileHeight = World::TileManager::getHeight({ newLoc.x, newLoc.y });
        const auto landZ = tileHeight.landHeight;
        const auto waterZ = tileHeight.waterHeight;

        if (waterZ != 0 && position.z >= waterZ && newLoc.z <= waterZ)
        {
            // We hit the water surface, create a splash effect.
            const auto splashPos = World::Pos3{ position.x, position.y, waterZ };

            Audio::playSound(Audio::SoundId::splash2, splashPos);
            Splash::create({ position.x, position.y, waterZ });

            EntityManager::freeEntity(this);
            return;
        }

        if (position.z >= landZ && newLoc.z <= landZ)
        {
            // We hit land surface apply bounce.
            accelerationZ = -accelerationZ;
            newLoc.z = landZ;
        }

        moveTo(newLoc);
        invalidateSprite();

        frame += kAnimationSpeed;
        if (frame >= kMaxSplashFrames)
        {
            frame = 0;
        }
    }

    // 0x00440B0A
    VehicleCrashParticle* VehicleCrashParticle::create(const World::Pos3& loc, const ColourScheme colourScheme)
    {
        auto t = static_cast<VehicleCrashParticle*>(EntityManager::createEntityMisc());
        if (t != nullptr)
        {
            t->colourScheme = colourScheme;
            t->spriteWidth = 8;
            t->spriteHeightNegative = 8;
            t->spriteHeightPositive = 8;
            t->baseType = EntityBaseType::effect;
            t->moveTo(loc);
            t->setSubType(EffectType::vehicleCrashParticle);

            const auto rand = gPrng1().randNext();
            t->frame = (rand & 0xFF) * 12;
            t->timeToLive = 140 + (rand & 0x7F);
            // rand value from 0 - 4 inclusive
            t->crashedSpriteBase = (((rand >> 23) & 0xFF) * 5) >> 8;

            t->accelerationX = static_cast<int16_t>(rand & 0xFFFF) * 4;
            t->accelerationY = static_cast<int16_t>((rand >> 16) & 0xFFFF) * 4;
            t->accelerationZ = ((rand >> 8) & 0xFFFF) * 4 + 0x10000;
            t->velocity = { 0, 0, 0 };
        }
        return t;
    }
}
