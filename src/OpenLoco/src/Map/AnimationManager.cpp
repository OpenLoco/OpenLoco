#include "AnimationManager.h"
#include "Animation.h"
#include "BuildingElement.h"
#include "Game.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "IndustryElement.h"
#include "RoadElement.h"
#include "SignalElement.h"
#include "StationElement.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <array>

using namespace OpenLoco::Interop;

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
            return;

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

    static bool callUpdateFunction(Animation& anim)
    {
        switch (anim.type)
        {
            case 0:
                return updateSignalAnimation(anim);
            case 1:
                return updateLevelCrossingAnimation(anim);
            case 2:
                return false;
            case 3:
                return updateIndustryAnimation1(anim);
            case 4:
                return updateIndustryAnimation2(anim);
            case 5:
                return updateBuildingAnimation1(anim);
            case 6:
                return updateBuildingAnimation2(anim);
            case 7:
                return updateAirportStationAnimation(anim);
            case 8:
                return updateDockStationAnimation(anim);
        }
        assert(false);
        return false;
    }

    // 0x004612EC
    void update()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            std::array<bool, Limits::kMaxAnimations> animsToRemove{};
            for (uint16_t i = 0; i < numAnimations(); ++i)
            {
                auto& animation = rawAnimations()[i];
                animsToRemove[i] = callUpdateFunction(animation);
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

    void registerHooks()
    {
        registerHook(
            0x004612A6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                createAnimation(regs.dh, { regs.ax, regs.cx }, regs.dl);
                regs = backup;
                return 0;
            });
    }
}
