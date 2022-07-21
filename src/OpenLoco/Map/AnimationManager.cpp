#include "AnimationManager.h"
#include "../Game.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"
#include "Animation.h"
#include <array>

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

    static bool callUpdateFunction(Animation& anim)
    {
        constexpr std::array<uint32_t, 9> _funcs = {
            0x0048950F,
            0x00479413,
            0x004BD528,
            0x00456E32,
            0x00456EEB,
            0x0042E4D4,
            0x0042E646,
            0x004939ED,
            0x004944B6,
        };
        registers regs;
        regs.ax = anim.pos.x;
        regs.cx = anim.pos.y;
        regs.dl = anim.baseZ;
        return call(_funcs[anim.type], regs) & X86_FLAG_CARRY;
    }

    // 0x004612EC
    void update()
    {
        if (Game::hasFlags(1u << 0))
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
            numAnimations() = last;
        }
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
