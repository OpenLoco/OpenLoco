#include "VehicleCrashEffect.h"
#include "Audio/Audio.h"
#include "Entities/EntityManager.h"
#include "Map/TileManager.h"
#include "SplashEffect.h"

#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
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
        accelerationZ -= 5041;

        // Apply air resistance
        accelerationX -= (accelerationX / 256);
        accelerationY -= (accelerationY / 256);
        accelerationZ -= (accelerationZ / 256);

        // Update velocity and position
        int32_t vx = velocity.x + accelerationX;
        int32_t vy = velocity.y + accelerationY;
        int32_t vz = velocity.z + accelerationZ;

        auto newLoc = position + World::Pos3{ vx >> 16, vy >> 16, vz >> 16 };

        velocity = World::Pos3{ vx & 0xFFFF, vy & 0xFFFF, vz & 0xFFFF };

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

        frame += 85;
        if (frame >= 3072)
        {
            frame = 0;
        }
    }
}
