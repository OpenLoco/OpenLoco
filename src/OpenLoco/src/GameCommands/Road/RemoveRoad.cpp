#include "RemoveRoad.h"
#include "Economy/Economy.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "RemoveRoadStation.h"
#include "SceneManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    using namespace World::TileManager;

    static loco_global<uint32_t, 0x0112C298> _dword_112C298;
    static loco_global<uint8_t, 0x0112C2CD> _byte_112C2CD;
    static loco_global<uint8_t, 0x0112C2D0> _byte_112C2D0;
    static loco_global<uint8_t, 0x0112C2CE> _byte_112C2CE;  // sequenceIndex
    static loco_global<uint8_t, 0x0112C2CF> _byte_112C2CF;  // roadId
    static loco_global<uint16_t, 0x0112C2B0> _word_112C2B0; // objectId
    static loco_global<uint8_t, 0x0112C2F3> _byte_112C2F3;  // flags
    static loco_global<uint8_t, 0x0112C2F7> _companyId;     // company id

    static World::RoadElement* getRoadElement(const World::Tile& tile, const RoadRemovalArgs& args, uint8_t flags)
    {
        const auto baseZ = args.pos.z / World::kSmallZStep;
        const auto companyId = SceneManager::isEditorMode() ? CompanyId::neutral : getUpdatingCompanyId();

        for (auto& element : tile)
        {
            auto* elRoad = element.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseZ() != baseZ)
            {
                continue;
            }
            if (elRoad->rotation() != args.rotation)
            {
                continue;
            }
            if (elRoad->sequenceIndex() != args.sequenceIndex)
            {
                continue;
            }
            if (elRoad->roadObjectId() != args.objectId)
            {
                continue;
            }
            if (elRoad->roadId() != args.roadId)
            {
                continue;
            }
            if ((elRoad->isGhost() && !(flags & Flags::ghost)) || (elRoad->isAiAllocated() && !(flags & Flags::aiAllocated)))
            {
                continue;
            }
            if ((flags & Flags::aiAllocated) && elRoad->owner() != companyId)
            {
                continue;
            }
            if (flags & Flags::aiAllocated)
            {
                return nullptr; // to 0x004776E3
            }
            if (elRoad->owner() != CompanyId::neutral)
            {
                return nullptr; // to 0x004776E3
            }
            if (elRoad->mods())
            {
                return nullptr; // to 0x004776E3
            }

            return elRoad;
        }

        return nullptr;
    }

    // 0x004775A5
    static uint32_t removeRoad(const RoadRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        // TODO: trying to use args instead of these; just for reference
        _dword_112C298 = 0;
        _byte_112C2CD = 0;
        _byte_112C2D0 = 0xFF;
        _byte_112C2CE = args.sequenceIndex;
        _byte_112C2CF = args.roadId;
        _word_112C2B0 = args.objectId;

        // TODO: already obsolete; just for reference
        _byte_112C2F3 = 0;
        if (flags & Flags::aiAllocated)
        {
            _byte_112C2F3 = _byte_112C2F3 | 0x10;
        }
        if (flags & Flags::ghost)
        {
            _byte_112C2F3 = _byte_112C2F3 | 0x20;
        }

        // 0x0047762D
        auto tile = World::TileManager::get(args.pos.x, args.pos.y);

        // This part was made redundant by a no-op write in Hooks.cpp
        // writeNop(0x004776DD, 6);
        /*
        if (auto* roadEl = getRoadElement(tile, args, flags); roadEl != nullptr)
        {
            if (!sub_431E6A(companyId, reinterpret_cast<const World::TileElement*>(&elRoad)))
            {
                return FAILURE;
            }

            setErrorText(StringIds::empty);

            auto nearest = TownManager::getClosestTownAndDensity(args.pos);
            if (nearest.has_value())
            {
                auto* town = TownManager::get(nearest->first);
                FormatArguments::common(town->name);
                setErrorText(StringIds::stringid_local_authority_wont_allow_removal_in_use);
                return FAILURE;
            }
        }
        */

        currency32_t totalCost = 0;

        // 0x004776E3
        if (auto* roadEl = getRoadElement(tile, args, flags); roadEl != nullptr)
        {
            auto* nextEl = roadEl->next();
            auto* nextRoad = nextEl->as<World::RoadElement>();
            if (nextRoad != nullptr)
            {
                auto* elStation = tile.roadStation(nextRoad->roadId(), nextRoad->rotation(), nextRoad->baseZ());
                if (elStation != nullptr)
                {
                    RoadStationRemovalArgs srArgs = {};
                    srArgs.pos = args.pos;
                    srArgs.rotation = args.rotation;
                    srArgs.roadId = args.roadId;
                    srArgs.index = args.sequenceIndex;
                    srArgs.roadObjectId = args.objectId;

                    auto stationRemovalRes = GameCommands::doCommand(srArgs, flags);
                    if (stationRemovalRes == FAILURE)
                    {
                        return FAILURE;
                    }

                    totalCost += stationRemovalRes;
                }
            }
        }

        // 0x004777EB

        return totalCost;
    }

    void removeRoad(registers& regs)
    {
        regs.ebx = removeRoad(RoadRemovalArgs(regs), regs.bl);
    }
}
