#include "TileClearance.h"
#include "BuildingElement.h"
#include "Economy/Economy.h"
#include "GameCommands/Buildings/RemoveBuilding.h"
#include "GameCommands/GameCommands.h"
#include "IndustryElement.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/BuildingObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TreeObject.h"
#include "RoadElement.h"
#include "ScenarioOptions.h"
#include "SignalElement.h"
#include "StationElement.h"
#include "SurfaceElement.h"
#include "TileManager.h"
#include "TrackElement.h"
#include "TreeElement.h"
#include "WallElement.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/Station.h"

using OpenLoco::World::TileManager::ElementPositionFlags;

namespace OpenLoco::World::TileClearance
{
    static loco_global<uint32_t, 0x00F00138> _F00138;
    static loco_global<TileElement*, 0x00F0015C> _F0015C;
    static loco_global<ElementPositionFlags, 0x00F00166> _constructAtElementPositionFlags;

    // 0x00462C8E
    void setCollisionErrorMessage(const World::TileElement& el)
    {
        switch (el.type())
        {
            case ElementType::surface:
            {
                GameCommands::setErrorText(StringIds::raise_or_lower_land_first);
                break;
            }
            case ElementType::track:
            {
                auto* elTrack = el.as<TrackElement>();
                if (elTrack == nullptr)
                {
                    return;
                }
                auto* trackObj = ObjectManager::get<TrackObject>(elTrack->trackObjectId());
                FormatArguments::common(trackObj->name);

                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }
            case ElementType::station:
            {
                auto* elStation = el.as<StationElement>();
                if (elStation == nullptr)
                {
                    return;
                }
                constexpr std::array<StringId, 4> kStationTypeNames = {
                    StringIds::capt_station,
                    StringIds::capt_station,
                    StringIds::capt_airport,
                    StringIds::capt_ship_port,
                };
                FormatArguments::common(kStationTypeNames[enumValue(elStation->stationType())]);

                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }

            case ElementType::signal:
            {
                FormatArguments::common(StringIds::capt_signal);
                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }
            case ElementType::building:
            {
                auto* elBuidling = el.as<BuildingElement>();
                if (elBuidling == nullptr)
                {
                    return;
                }
                auto* buildingObj = elBuidling->getObject();
                FormatArguments::common(buildingObj->name);

                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }
            case ElementType::tree:
            {
                auto* elTree = el.as<TreeElement>();
                if (elTree == nullptr)
                {
                    return;
                }
                auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                FormatArguments::common(treeObj->name);

                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }
            case ElementType::wall:
            {
                GameCommands::setErrorText(StringIds::object_in_the_way);
                break;
            }
            case ElementType::road:
            {
                auto* elRoad = el.as<RoadElement>();
                if (elRoad == nullptr)
                {
                    return;
                }
                auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                FormatArguments::common(roadObj->name);

                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }
            case ElementType::industry:
            {
                auto* elIndustry = el.as<IndustryElement>();
                if (elIndustry == nullptr)
                {
                    return;
                }
                auto* industry = elIndustry->industry();
                FormatArguments::common(industry->name, industry->town);

                GameCommands::setErrorText(StringIds::string_id_in_the_way);
                break;
            }
        }
    }

    constexpr bool isCollisionResult(ClearFuncResult res)
    {
        return res == ClearFuncResult::collision || res == ClearFuncResult::collisionErrorSet;
    }

    // 0x00462BB3
    // Vanilla function would return true with input esi not modified
    // Vanilla function would return false with esi set to 0xFFFFFFFF if error text set or problem element if not set
    static ClearFuncResult aiCompanyAboutToBuildCheck(const World::TileElement& el)
    {
        auto* elSurface = el.as<SurfaceElement>();

        // 0x00462C02
        auto returnFunc = [](const CompanyId owner) -> ClearFuncResult {
            const auto updatingId = GameCommands::getUpdatingCompanyId();
            if (updatingId == CompanyId::neutral || !CompanyManager::isPlayerCompany(updatingId))
            {
                GameCommands::setErrorText(StringIds::another_company_is_about_to_build_here);
                return ClearFuncResult::collisionErrorSet;
            }
            if (owner == updatingId || CompanyManager::isPlayerCompany(owner))
            {
                return ClearFuncResult::collision;
            }
            auto* company = CompanyManager::get(owner);
            if ((company->challengeFlags & CompanyFlags::unk2) != CompanyFlags::none)
            {
                GameCommands::setErrorText(StringIds::another_company_is_about_to_build_here);
                return ClearFuncResult::collisionErrorSet;
            }
            // Modification from vanilla
            company->challengeFlags |= CompanyFlags::unk1;
            return ClearFuncResult::noCollision;
        };

        if (elSurface == nullptr)
        {
            CompanyId owner = TileManager::getTileOwner(el);
            if (owner == CompanyId::null)
            {
                return ClearFuncResult::noCollision;
            }

            return returnFunc(owner);
        }
        else
        {
            auto* elp = &el;
            while (!elp->isLast())
            {
                elp = elp->next();
                if (!elp->isAiAllocated())
                {
                    continue;
                }
                const auto owner = TileManager::getTileOwner(*elp);
                if (owner == CompanyId::null)
                {
                    continue;
                }

                return returnFunc(owner);
            }
            GameCommands::setErrorText(StringIds::another_company_is_about_to_build_here);
            return ClearFuncResult::collisionErrorSet;
        }
    }

    // 0x00462B4F
    static ClearFuncResult callClearFunction(TileElement& el, const std::function<ClearFuncResult(TileElement& el)>& clearFunc)
    {
        if (!clearFunc)
        {
            return ClearFuncResult::collision;
        }
        _F0015C = nullptr;
        return clearFunc(el);
    };

    enum class BuildingCollisionType : bool
    {
        standard,
        anyHeight, // If the building/industry/tree/dock/airport is at any height on the tile this is a collision
    };

    // 0x0046297D
    static ClearFuncResult canConstructAtCheckSurfaceElement(uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, const std::function<ClearFuncResult(TileElement& el)>& clearFunc, TileElement& el, const SurfaceElement& elSurface)
    {
        if (elSurface.isAiAllocated())
        {
            if (auto res = aiCompanyAboutToBuildCheck(el);
                res != ClearFuncResult::noCollision)
            {
                return res;
            }
        }
        const auto waterZ = elSurface.water() * kMicroToSmallZStep;
        if (waterZ != 0)
        {
            if (clearZ > elSurface.clearZ() && baseZ < waterZ + 4)
            {
                _constructAtElementPositionFlags = _constructAtElementPositionFlags | ElementPositionFlags::partiallyUnderwater;
                if (baseZ < waterZ)
                {
                    _constructAtElementPositionFlags = _constructAtElementPositionFlags | ElementPositionFlags::underwater;
                    if (clearZ > waterZ)
                    {
                        if (clearFunc)
                        {
                            _F0015C = nullptr;
                            if (auto res = clearFunc(el); isCollisionResult(res))
                            {
                                if (res == ClearFuncResult::collision)
                                {
                                    GameCommands::setErrorText(StringIds::cannot_build_partly_above_below_water);
                                }
                                return ClearFuncResult::collisionErrorSet;
                            }
                        }
                        else
                        {
                            GameCommands::setErrorText(StringIds::cannot_build_partly_above_below_water);
                            return ClearFuncResult::collisionErrorSet;
                        }
                    }
                }
            }
        }
        if (qt.getZQuarterOccupied() == 0xF)
        {
            return ClearFuncResult::noCollision;
        }

        if (clearZ <= elSurface.baseZ())
        {
            _constructAtElementPositionFlags = _constructAtElementPositionFlags | ElementPositionFlags::underground;
            _constructAtElementPositionFlags = _constructAtElementPositionFlags & ~(ElementPositionFlags::aboveGround);
            return ClearFuncResult::noCollision;
        }
        else
        {
            auto northZ = elSurface.baseZ();
            auto eastZ = northZ;
            auto southZ = northZ;
            auto westZ = northZ;
            const auto slope = elSurface.slope();
            if (slope & SurfaceSlope::CornerUp::north)
            {
                northZ += kSmallZStep;
                if (slope == (SurfaceSlope::CornerDown::south | SurfaceSlope::doubleHeight))
                {
                    northZ += kSmallZStep;
                }
            }
            if (slope & SurfaceSlope::CornerUp::east)
            {
                eastZ += kSmallZStep;
                if (slope == (SurfaceSlope::CornerDown::west | SurfaceSlope::doubleHeight))
                {
                    eastZ += kSmallZStep;
                }
            }
            if (slope & SurfaceSlope::CornerUp::south)
            {
                southZ += kSmallZStep;
                if (slope == (SurfaceSlope::CornerDown::north | SurfaceSlope::doubleHeight))
                {
                    southZ += kSmallZStep;
                }
            }
            if (slope & SurfaceSlope::CornerUp::west)
            {
                westZ += kSmallZStep;
                if (slope == (SurfaceSlope::CornerDown::east | SurfaceSlope::doubleHeight))
                {
                    westZ += kSmallZStep;
                }
            }
            const auto doubleHeight = baseZ + 8;

            const auto baseQuarter = qt.getBaseQuarterOccupied();
            const auto zQuarter = qt.getZQuarterOccupied();
            if ((!(baseQuarter & 0b0001) || ((zQuarter & 0b0001 || baseZ >= northZ) && doubleHeight >= northZ))
                && (!(baseQuarter & 0b0010) || ((zQuarter & 0b0010 || baseZ >= eastZ) && doubleHeight >= eastZ))
                && (!(baseQuarter & 0b0100) || ((zQuarter & 0b0100 || baseZ >= southZ) && doubleHeight >= southZ))
                && (!(baseQuarter & 0b1000) || ((zQuarter & 0b1000 || baseZ >= westZ) && doubleHeight >= westZ)))
            {
                return ClearFuncResult::noCollision;
            }
            return callClearFunction(el, clearFunc);
        }
    }

    // 0x00462AA4
    static ClearFuncResult canConstructAtCheckNonSurfaceElement(const uint8_t baseZ, const uint8_t clearZ, const QuarterTile& qt, const BuildingCollisionType flags, const std::function<ClearFuncResult(TileElement& el)>& clearFunc, TileElement& el)
    {
        if (flags == BuildingCollisionType::anyHeight)
        {
            if (el.type() == ElementType::tree || el.type() == ElementType::building || el.type() == ElementType::industry)
            {
                return callClearFunction(el, clearFunc);
            }
            const auto* elStation = el.as<StationElement>();
            if (elStation != nullptr)
            {
                if (elStation->stationType() == StationType::airport || elStation->stationType() == StationType::docks)
                {
                    return callClearFunction(el, clearFunc);
                }
            }
        }
        if (baseZ >= el.clearZ())
        {
            return ClearFuncResult::noCollision;
        }
        if (clearZ <= el.baseZ())
        {
            return ClearFuncResult::noCollision;
        }
        if (el.isGhost())
        {
            return ClearFuncResult::noCollision;
        }
        if ((el.occupiedQuarter() & qt.getBaseQuarterOccupied()) == 0)
        {
            return ClearFuncResult::noCollision;
        }
        if (!el.isAiAllocated())
        {
            return callClearFunction(el, clearFunc);
        }
        if (clearFunc)
        {
            _F0015C = nullptr;
            if (auto res = clearFunc(el);
                !isCollisionResult(res))
            {
                return res;
            }
        }
        return aiCompanyAboutToBuildCheck(el);
    }

    // 0x00462937
    static bool canConstructAtWithClear(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, BuildingCollisionType flags, std::function<ClearFuncResult(TileElement& el)> clearFunc)
    {
        _constructAtElementPositionFlags = ElementPositionFlags::aboveGround;
        if (!TileManager::drawableCoords(pos))
        {
            GameCommands::setErrorText(StringIds::off_edge_of_map);
            return false;
        }

        bool collisionRemoved = false;
        do
        {
            collisionRemoved = false;
            const auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                const auto* elSurface = el.as<SurfaceElement>();
                const auto res = elSurface == nullptr ? canConstructAtCheckNonSurfaceElement(baseZ, clearZ, qt, flags, clearFunc, el)
                                                      : canConstructAtCheckSurfaceElement(baseZ, clearZ, qt, clearFunc, el, *elSurface);
                switch (res)
                {
                    case ClearFuncResult::noCollision:
                        break;
                    case ClearFuncResult::allCollisionsRemoved:
                        return true;
                    case ClearFuncResult::collision:
                        setCollisionErrorMessage(el);
                        return false;
                    case ClearFuncResult::collisionRemoved:
                        collisionRemoved = true;
                        break;
                    case ClearFuncResult::collisionErrorSet:
                        return false;
                }
                if (collisionRemoved)
                {
                    break;
                }
            }
        } while (collisionRemoved);
        return true;
    }

    // 0x00462926
    static bool canConstructAtWithClearLegacy(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, BuildingCollisionType flags, uintptr_t clearFunctionLegacy)
    {
        if (clearFunctionLegacy == 0xFFFFFFFFU)
        {
            return canConstructAtWithClear(pos, baseZ, clearZ, qt, flags, {});
        }
        // NOTE: When implementing a clear function you should ensure you follow all of this!
        // especially the collisionRemoved/allCollisionsRemoved parts
        auto functionWrapper = [clearFunctionLegacy](TileElement& el) -> ClearFuncResult {
            registers regs{};
            regs.esi = X86Pointer(&el);
            bool hasCollision = call(clearFunctionLegacy, regs) & Interop::X86_FLAG_CARRY;
            if (hasCollision)
            {
                if (regs.esi == -1)
                {
                    return ClearFuncResult::collisionErrorSet;
                }
                else
                {
                    return ClearFuncResult::collision;
                }
            }
            else
            {
                if (_F0015C == nullptr)
                {
                    return ClearFuncResult::noCollision;
                }
                if (_F0015C == TileManager::kInvalidTile)
                {
                    // This is a no collision as it has removed all the collisions
                    return ClearFuncResult::allCollisionsRemoved;
                }
                // reset tile iterator to _F0015C (its saying we have removed a tile so pointer is stale)
                return ClearFuncResult::collisionRemoved;
            }
        };

        return canConstructAtWithClear(pos, baseZ, clearZ, qt, flags, functionWrapper);
    }

    // 0x00462908
    bool applyClearAtAllHeights(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy)
    {
        return canConstructAtWithClearLegacy(pos, baseZ, clearZ, qt, BuildingCollisionType::anyHeight, clearFunctionLegacy);
    }

    bool applyClearAtAllHeights(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc)
    {
        return canConstructAtWithClear(pos, baseZ, clearZ, qt, BuildingCollisionType::anyHeight, clearFunc);
    }

    // 0x00462917
    bool applyClearAtStandardHeight(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy)
    {
        return canConstructAtWithClearLegacy(pos, baseZ, clearZ, qt, BuildingCollisionType::standard, clearFunctionLegacy);
    }

    bool applyClearAtStandardHeight(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc)
    {
        return canConstructAtWithClear(pos, baseZ, clearZ, qt, BuildingCollisionType::standard, clearFunc);
    }

    // 0x00462926
    bool canConstructAt(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt)
    {
        return canConstructAtWithClear(pos, baseZ, clearZ, qt, BuildingCollisionType::standard, {});
    }

    // 0x00469E07, 0x00468949, 0x004C4DAD, 0x0042D5E5, 0x0049434F
    static ClearFuncResult tileClearFunction(World::TileElement& el, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost, bool defaultCollision)
    {
        switch (el.type())
        {
            case ElementType::surface:
                return ClearFuncResult::noCollision;
            case ElementType::tree:
            {
                auto* elTree = el.as<TreeElement>();
                if (elTree == nullptr)
                {
                    return ClearFuncResult::noCollision;
                }
                return clearTreeCollision(*elTree, pos, flags, cost);
            }
            case ElementType::building:
            {
                auto* elBuilding = el.as<BuildingElement>();
                if (elBuilding == nullptr)
                {
                    return ClearFuncResult::noCollision;
                }
                return clearBuildingCollision(*elBuilding, pos, removedBuildings, flags, cost);
            }
            default:
                return defaultCollision ? ClearFuncResult::collision : ClearFuncResult::noCollision;
        }
    };

    ClearFuncResult clearWithDefaultCollision(World::TileElement& el, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost)
    {
        return tileClearFunction(el, pos, removedBuildings, flags, cost, true);
    }

    ClearFuncResult clearWithoutDefaultCollision(World::TileElement& el, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost)
    {
        return tileClearFunction(el, pos, removedBuildings, flags, cost, false);
    }

    ClearFuncResult clearBuildingCollision(World::BuildingElement& elBuilding, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost)
    {
        auto* buildingObj = elBuilding.getObject();
        if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
        {
            return ClearFuncResult::collision;
        }

        const auto buildingStart = World::Pos3{
            pos - World::kOffsets[elBuilding.sequenceIndex()], elBuilding.baseHeight()
        };
        if (removedBuildings.count(buildingStart) != 0)
        {
            return ClearFuncResult::noCollision;
        }
        removedBuildings.insert(buildingStart);

        World::TileManager::setRemoveElementPointerChecker(reinterpret_cast<TileElement&>(elBuilding));
        uint8_t removeBuildingFlags = flags;
        if ((flags & GameCommands::Flags::apply) || removedBuildings.size() != 1)
        {
            removeBuildingFlags |= GameCommands::Flags::flag_7;
        }
        if (flags & (GameCommands::Flags::ghost | GameCommands::Flags::aiAllocated))
        {
            removeBuildingFlags &= ~(GameCommands::Flags::ghost | GameCommands::Flags::aiAllocated | GameCommands::Flags::apply);
        }
        GameCommands::BuildingRemovalArgs args{};
        args.pos = buildingStart;
        Interop::registers regs = static_cast<Interop::registers>(args);
        regs.bl = removeBuildingFlags;
        // We should probably call doCommand here but then it gets messy with the costs
        // look into changing this in the future.
        GameCommands::removeBuilding(regs);
        const auto buildingCost = static_cast<currency32_t>(regs.ebx);
        if (static_cast<uint32_t>(buildingCost) == GameCommands::FAILURE)
        {
            return ClearFuncResult::collisionErrorSet;
        }
        if (flags & GameCommands::Flags::apply)
        {
            Scenario::getOptions().madeAnyChanges = 1;
        }
        cost += buildingCost;

        if (!(flags & GameCommands::Flags::apply) || (flags & (GameCommands::Flags::ghost | GameCommands::Flags::aiAllocated)))
        {
            return ClearFuncResult::noCollision;
        }
        if (World::TileManager::wasRemoveOnLastElement())
        {
            return ClearFuncResult::allCollisionsRemoved;
        }
        return ClearFuncResult::collisionRemoved;
    }

    ClearFuncResult clearTreeCollision(World::TreeElement& elTree, const World::Pos2 pos, const uint8_t flags, currency32_t& cost)
    {
        auto* treeObj = ObjectManager::get<TreeObject>(elTree.treeObjectId());
        cost += Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);

        if ((flags & (GameCommands::Flags::ghost | GameCommands::Flags::aiAllocated)) || !(flags & GameCommands::Flags::apply))
        {
            return ClearFuncResult::noCollision;
        }

        World::TileManager::setRemoveElementPointerChecker(reinterpret_cast<TileElement&>(elTree));
        World::TileManager::removeTree(elTree, GameCommands::Flags::apply, pos);
        Scenario::getOptions().madeAnyChanges = 1;
        if (World::TileManager::wasRemoveOnLastElement())
        {
            return ClearFuncResult::allCollisionsRemoved;
        }
        return ClearFuncResult::collisionRemoved;
    }

    ElementPositionFlags getPositionFlags()
    {
        return *_constructAtElementPositionFlags;
    }

    void registerHooks()
    {
        registerHook(
            0x00462937,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backupRegs = regs;
                QuarterTile qt{ static_cast<uint8_t>(regs.bl) };
                const uint32_t legacyFunction = _F00138;
                const auto flags = ((_constructAtElementPositionFlags & ElementPositionFlags::anyHeightBuildingCollisions) != ElementPositionFlags::none) ? BuildingCollisionType::anyHeight : BuildingCollisionType::standard;
                auto res = canConstructAtWithClearLegacy({ regs.ax, regs.cx }, regs.dl, regs.dh, qt, flags, legacyFunction);
                regs = backupRegs;
                return res ? 0 : X86_FLAG_CARRY;
            });
    }
}
