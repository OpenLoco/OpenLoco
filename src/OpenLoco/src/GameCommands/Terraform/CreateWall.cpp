#include "CreateWall.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/QuarterTile.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/WallObject.h"
#include "OpenLoco.h"
#include "Scenario.h"
#include "ScenarioOptions.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    struct EdgeSlopeMapEntry
    {
        EdgeSlope slope;
        bool isElevated;
    };

    namespace EdgeSlopeMap
    {
        constexpr auto none = EdgeSlopeMapEntry{ EdgeSlope::none, false };
        constexpr auto downwards = EdgeSlopeMapEntry{ EdgeSlope::downwards, false };
        constexpr auto upwards = EdgeSlopeMapEntry{ EdgeSlope::upwards, false };
        constexpr auto downwardsElevated = EdgeSlopeMapEntry{ EdgeSlope::downwards, true };
        constexpr auto upwardsElevated = EdgeSlopeMapEntry{ EdgeSlope::upwards, true };
        constexpr auto elevated = EdgeSlopeMapEntry{ EdgeSlope::none, true };
    }

    // clang-format off
    constexpr std::array<std::array<EdgeSlopeMapEntry, 4>, 32> edgeWallMapping = { {
        // top-right                       bottom-right                     bottom-left                      top-left
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::upwards,           EdgeSlopeMap::downwards,         EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::upwards,           EdgeSlopeMap::downwards         },
        { EdgeSlopeMap::none,              EdgeSlopeMap::upwards,           EdgeSlopeMap::elevated,          EdgeSlopeMap::downwards         },
        { EdgeSlopeMap::downwards,         EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::upwards           },
        { EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards,           EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards           },
        { EdgeSlopeMap::downwards,         EdgeSlopeMap::none,              EdgeSlopeMap::upwards,           EdgeSlopeMap::elevated          },
        { EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards,           EdgeSlopeMap::elevated,          EdgeSlopeMap::elevated          },
        { EdgeSlopeMap::upwards,           EdgeSlopeMap::downwards,         EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::upwards,           EdgeSlopeMap::elevated,          EdgeSlopeMap::downwards,         EdgeSlopeMap::none              },
        { EdgeSlopeMap::upwards,           EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards,           EdgeSlopeMap::downwards         },
        { EdgeSlopeMap::upwards,           EdgeSlopeMap::elevated,          EdgeSlopeMap::elevated,          EdgeSlopeMap::downwards         },
        { EdgeSlopeMap::elevated,          EdgeSlopeMap::downwards,         EdgeSlopeMap::none,              EdgeSlopeMap::upwards           },
        { EdgeSlopeMap::elevated,          EdgeSlopeMap::elevated,          EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards           },
        { EdgeSlopeMap::elevated,          EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards,           EdgeSlopeMap::elevated          },
        { EdgeSlopeMap::elevated,          EdgeSlopeMap::elevated,          EdgeSlopeMap::elevated,          EdgeSlopeMap::elevated          },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards,           EdgeSlopeMap::upwardsElevated,   EdgeSlopeMap::downwardsElevated },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::upwards,           EdgeSlopeMap::upwardsElevated,   EdgeSlopeMap::downwardsElevated, EdgeSlopeMap::downwards         },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
        { EdgeSlopeMap::upwardsElevated,   EdgeSlopeMap::downwardsElevated, EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards           },
        { EdgeSlopeMap::downwardsElevated, EdgeSlopeMap::downwards,         EdgeSlopeMap::upwards,           EdgeSlopeMap::upwardsElevated   },
        { EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none,              EdgeSlopeMap::none              },
    } };
    // clang-format on

    // 0x004C4A3B
    static bool canConstructWall(World::Pos3 pos, SmallZ baseZ, SmallZ clearZ, uint8_t targetEdge)
    {
        if (!drawableCoords(World::Pos2(pos)))
        {
            setErrorText(StringIds::off_edge_of_map);
            return false;
        }

        for (auto& el : TileManager::get(pos))
        {
            if (el.type() == ElementType::surface)
            {
                continue;
            }
            if (baseZ >= el.clearZ())
            {
                continue;
            }
            if (clearZ <= el.baseZ())
            {
                continue;
            }

            if (el.type() == ElementType::wall)
            {
                auto* wallEl = el.as<WallElement>();
                if (wallEl != nullptr && wallEl->rotation() == targetEdge)
                {
                    TileClearance::setCollisionErrorMessage(el);
                    return false;
                }
                continue;
            }

            if (!el.occupiedQuarter())
            {
                continue;
            }

            if (el.type() == ElementType::tree)
            {
                continue;
            }

            TileClearance::setCollisionErrorMessage(el);
            return false;
        }

        return true;
    }

    // 0x004C436C
    static uint32_t createWall(const WallPlacementArgs& args, const uint8_t flags)
    {
        getLegacyReturnState().lastPlacedWall = nullptr;
        setExpenditureType(ExpenditureType::Construction);

        auto zPos = args.pos.z;
        if (zPos == 0)
        {
            auto tileHeight = World::TileManager::getHeight(args.pos);
            zPos = tileHeight.landHeight;
        }
        setPosition(World::Pos3(args.pos.x + World::kTileSize / 2, args.pos.y + World::kTileSize / 2, zPos));

        if (!World::validCoords(args.pos))
        {
            return FAILURE;
        }

        auto* surface = World::TileManager::get(args.pos).surface();
        if (surface == nullptr)
        {
            return FAILURE;
        }

        auto slopeFlags = EdgeSlope::none;
        auto targetHeight = args.pos.z;
        if (targetHeight == 0)
        {
            targetHeight = surface->baseHeight();
            auto edge = args.rotation & 3;
            const auto& edgeMapping = edgeWallMapping[surface->slope()][edge];
            slopeFlags = edgeMapping.slope;
            if (edgeMapping.isElevated)
            {
                targetHeight += 16;
            }
        }

        const auto targetBaseZ = targetHeight / kSmallZStep;

        if (surface->water() && targetBaseZ < surface->water() * kMicroToSmallZStep)
        {
            setErrorText(StringIds::cant_build_this_underwater);
            return FAILURE;
        }

        if (targetBaseZ < surface->baseZ())
        {
            setErrorText(StringIds::error_can_only_build_above_ground);
            return FAILURE;
        }

        if ((slopeFlags & (EdgeSlope::upwards | EdgeSlope::downwards)) == EdgeSlope::none)
        {
            const auto testHeight = surface->baseZ() + kSmallZStep;

            // Test placement edges to ensure we don't build partially underground
            for (auto i = 2; i <= 3; i++)
            {
                auto testEdge = (args.rotation + i) & 3;
                if (surface->slope() & (1 << testEdge))
                {
                    if (targetBaseZ < testHeight)
                    {
                        setErrorText(StringIds::error_can_only_build_above_ground);
                        return FAILURE;
                    }

                    if (surface->isSlopeDoubleHeight())
                    {
                        testEdge = (testEdge - 1) & 3;
                        if (surface->slope() & (1 << testEdge))
                        {
                            testEdge = (testEdge + 2) & 3;
                            if (surface->slope() & (1 << testEdge))
                            {
                                if (targetBaseZ < testHeight + kSmallZStep)
                                {
                                    setErrorText(StringIds::error_can_only_build_above_ground);
                                    return FAILURE;
                                }
                            }
                        }
                    }
                }
            }
        }

        auto clearZ = targetBaseZ;

        // TODO: fold into previous block; left for now to match IDA
        auto* wallObj = ObjectManager::get<WallObject>(args.type);
        if ((slopeFlags & (EdgeSlope::upwards | EdgeSlope::downwards)) != EdgeSlope::none)
        {
            if ((wallObj->flags & WallObjectFlags::onlyOnLevelLand) != WallObjectFlags::none)
            {
                setErrorText(StringIds::can_only_build_this_on_level_land);
                return FAILURE;
            }

            clearZ += kSmallZStep;
        }
        clearZ += wallObj->height;

        if (!canConstructWall(args.pos, targetBaseZ, clearZ, args.rotation))
        {
            return FAILURE;
        }

        if (!TileManager::checkFreeElementsAndReorganise())
        {
            // Error message set in checkFreeElementsAndReorganise
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        auto* wall = TileManager::insertElement<WallElement>(args.pos, targetBaseZ, 0);
        if (wall == nullptr)
        {
            return FAILURE;
        }

        wall->setClearZ(clearZ);
        wall->setRotation(args.rotation);
        wall->setSlopeFlags(slopeFlags);
        wall->setPrimaryColour(args.primaryColour);
        wall->setSecondaryColour(args.secondaryColour);
        wall->setWallObjectId(args.type);

        if ((wallObj->flags & WallObjectFlags::hasTertiaryColour) != WallObjectFlags::none)
        {
            wall->setTertiaryColour(args.tertiaryColour);
        }

        if (flags & Flags::ghost)
        {
            wall->setGhost(true);
        }

        getLegacyReturnState().lastPlacedWall = wall;

        Ui::ViewportManager::invalidate(args.pos, wall->baseHeight(), wall->baseHeight() + 72, ZoomLevel::half);

        Scenario::getOptions().madeAnyChanges = 1;

        return 0;
    }

    void createWall(registers& regs)
    {
        WallPlacementArgs args(regs);
        regs.ebx = createWall(args, regs.bl);
    }
}
