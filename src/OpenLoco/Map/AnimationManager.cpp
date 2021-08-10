#include "AnimationManager.h"
#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::AnimationManager
{

#pragma pack(push, 1)
    struct TileAnimation
    {
        uint8_t baseZ;
        uint8_t type;
        Map::Pos2 pos;
    };
    static_assert(sizeof(TileAnimation) == 6);
#pragma pack(pop)

    constexpr size_t maxAnimations = 0x2000;

    static loco_global<TileAnimation[maxAnimations], 0x0094C6DC> _animations;
    static loco_global<uint16_t, 0x00525F6C> _numAnimations;
    // 0x004612A6
    void createAnimation(uint8_t type, const Pos2& pos, tile_coord_t baseZ)
    {
        if (_numAnimations >= maxAnimations)
            return;

        for (size_t i = 0; i < _numAnimations; i++)
        {
            auto& animation = _animations[i];
            if (animation.type == type && animation.pos == pos && animation.baseZ == baseZ)
            {
                return;
            }
        }

        auto& newAnimation = _animations[_numAnimations++];
        newAnimation.baseZ = baseZ;
        newAnimation.type = type;
        newAnimation.pos = pos;
    }

    // 0x00461166
    void resetAnimations()
    {
        _numAnimations = 0;
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
