#include "SignalElement.h"
#include "Animation.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainSignalObject.h"
#include "ScenarioManager.h"
#include "TileManager.h"
#include "ViewportManager.h"

namespace OpenLoco::World
{
    struct AnimResult
    {
        bool hasAnimation = false;
        bool shouldInvalidate = false;
    };
    static AnimResult updateSignalAnimationSide(SignalElement::Side& side)
    {
        AnimResult res{};
        if (side.hasSignal())
        {
            const auto* signalObj = ObjectManager::get<TrainSignalObject>(side.signalObjectId());
            const auto targetFrame = std::min(side.getUnk4() * 3, signalObj->numFrames - 1);
            if (side.frame() != targetFrame)
            {
                res.hasAnimation = true;
                if (ScenarioManager::getScenarioTicks() & signalObj->animationSpeed)
                {
                    uint8_t newFrame = side.frame() + 1;
                    if (side.frame() >= targetFrame)
                    {
                        newFrame = side.frame() - 1;
                        if (signalObj->hasFlags(TrainSignalObjectFlags::unk2))
                        {
                            newFrame = std::max<uint8_t>(newFrame, 1U);
                        }
                    }
                    side.setFrame(newFrame);
                    res.shouldInvalidate = true;
                }
            }
        }
        return res;
    }
    // 0x0048950F
    bool updateSignalAnimation(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
        for (auto& el : tile)
        {
            auto* elSignal = el.as<SignalElement>();
            if (elSignal == nullptr)
            {
                continue;
            }
            if (elSignal->baseZ() != anim.baseZ)
            {
                continue;
            }
            auto leftRes = updateSignalAnimationSide(elSignal->getLeft());
            auto rightRes = updateSignalAnimationSide(elSignal->getRight());

            if (leftRes.shouldInvalidate || rightRes.shouldInvalidate)
            {
                Ui::ViewportManager::invalidate(anim.pos, el.baseHeight(), el.clearHeight(), ZoomLevel::half);
            }

            return !(leftRes.hasAnimation || rightRes.hasAnimation);
        }
        return true;
    }
}
