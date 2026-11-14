#include "Map/AnimationManager.h"
#include "Game.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Map/Animation.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"

#include <array>

namespace OpenLoco::World::AnimationManager
{
    static auto& rawAnimations()
    {
        return getGameState().animations;
    }

    static auto& numAnimations()
    {
        return getGameState().numMapAnimations;
    }

    // 0x004612A6
    void createAnimation(uint8_t type, const Pos2& pos, tile_coord_t baseZ)
    {
        if (numAnimations() >= Limits::kMaxAnimations)
        {
            return;
        }

        for (size_t i = 0; i < numAnimations(); i++)
        {
            auto& animation = rawAnimations()[i];
            if (animation.type == type && animation.pos == pos && animation.baseZ == baseZ)
            {
                return;
            }
        }

        auto& newAnimation = rawAnimations()[numAnimations()++];
        newAnimation.baseZ = baseZ;
        newAnimation.type = type;
        newAnimation.pos = pos;
    }

    // 0x00461166
    void reset()
    {
        numAnimations() = 0;
    }

    static bool callTickFunction(Animation& anim)
    {
        switch (anim.type)
        {
            case 0:
                return tickSignalAnimation(anim);
            case 1:
                return tickLevelCrossingAnimation(anim);
            case 2:
                return false;
            case 3:
                return tickIndustryContinuousAnimation(anim);
            case 4:
                return tickIndustryRandomAnimation(anim);
            case 5:
                return tickBuildingAnimation1(anim);
            case 6:
                return tickBuildingAnimation2(anim);
            case 7:
                return tickAirportStationAnimation(anim);
            case 8:
                return tickDockStationAnimation(anim);
        }
        assert(false);
        return false;
    }

    // 0x004612EC
    void tick()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            std::array<bool, Limits::kMaxAnimations> animsToRemove{};
            for (uint16_t i = 0; i < numAnimations(); ++i)
            {
                auto& animation = rawAnimations()[i];
                animsToRemove[i] = callTickFunction(animation);
            }

            // Remove animations that are no longer required
            uint16_t last = 0;
            for (uint16_t i = 0; i < numAnimations(); ++i, ++last)
            {
                while (animsToRemove[i] && i < numAnimations())
                {
                    ++i;
                }
                if (i >= numAnimations())
                {
                    break;
                }
                rawAnimations()[last] = rawAnimations()[i];
            }

            // For vanilla binary compatibility copy the old last entry across all garbage entries
            auto repCount = numAnimations() - last;
            std::fill_n(std::next(std::begin(rawAnimations()), last), repCount, rawAnimations()[numAnimations() - 1]);
            // Above to be deleted when confirmed matching

            numAnimations() = last;
        }
    }
}
