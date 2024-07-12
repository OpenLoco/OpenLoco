#include "StationElement.h"
#include "Animation.h"
#include "Objects/AirportObject.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
#include "ScenarioManager.h"
#include "TileManager.h"
#include "ViewportManager.h"
#include "World/Station.h"

namespace OpenLoco::World
{
    template<typename Object>
    bool updateStationAnimation(const Animation& anim, StationType stationType)
    {
        auto tile = TileManager::get(anim.pos);
        for (auto& el : tile)
        {
            auto* elStation = el.as<StationElement>();
            if (elStation == nullptr)
            {
                continue;
            }
            if (elStation->baseZ() != anim.baseZ)
            {
                continue;
            }
            if (elStation->stationType() != stationType)
            {
                continue;
            }

            const auto* obj = ObjectManager::get<Object>(elStation->objectId());
            auto buildingParts = obj->getBuildingParts(elStation->buildingType());
            bool hasAnimation = false;
            uint8_t animSpeed = std::numeric_limits<uint8_t>::max();
            for (auto& part : buildingParts)
            {
                auto& partAnim = obj->buildingPartAnimations[part];
                if (partAnim.numFrames > 1)
                {
                    hasAnimation = true;
                    animSpeed = std::min<uint8_t>(animSpeed, partAnim.animationSpeed & ~(1 << 7));
                }
            }
            if (!hasAnimation)
            {
                return true;
            }
            const auto speedMask = ((1 << animSpeed) - 1);
            if (!(ScenarioManager::getScenarioTicks() & speedMask))
            {
                Ui::ViewportManager::invalidate(anim.pos, el.baseHeight(), el.clearHeight(), ZoomLevel::quarter);
            }
            return false;
        }
        return true;
    }

    // 0x004944B6
    bool updateDockStationAnimation(const Animation& anim)
    {
        return updateStationAnimation<DockObject>(anim, StationType::docks);
    }

    // 0x004939ED
    bool updateAirportStationAnimation(const Animation& anim)
    {
        return updateStationAnimation<AirportObject>(anim, StationType::airport);
    }
}
