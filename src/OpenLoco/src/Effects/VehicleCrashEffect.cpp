#include "VehicleCrashEffect.h"
#include "Audio/Audio.h"
#include "Entities/EntityManager.h"
#include "Map/TileManager.h"
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
}
