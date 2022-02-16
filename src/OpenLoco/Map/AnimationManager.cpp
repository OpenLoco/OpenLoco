#include "AnimationManager.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"
#include "Animation.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::AnimationManager
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

    // 0x004612EC
    void update()
    {
        call(0x004612EC);
    }

    void registerHooks()
    {
        registerHook(
            0x004612A6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                createAnimation(regs.dh, { regs.ax, regs.cx }, regs.dl);
                return 0;
            });
    }
}
