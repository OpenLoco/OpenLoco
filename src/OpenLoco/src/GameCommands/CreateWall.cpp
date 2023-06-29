#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
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
#include "S5/S5.h"
#include "Scenario.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    // clang-format off
    enum class EdgeSlope : uint8_t
    {
        none              = 0U,
        elevated          = (1 << 0),
        upwards           = (1 << 6),
        downwards         = (1 << 7),
        upwardsElevated   = EdgeSlope::upwards | EdgeSlope::elevated,
        downwardsElevated = EdgeSlope::downwards | EdgeSlope::elevated,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(EdgeSlope);

    constexpr std::array<std::array<EdgeSlope, 4>, 32> edgeWallMapping = { {
        // top-right                    bottom-right                  bottom-left                   top-left
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::upwards,           EdgeSlope::downwards,         EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::upwards,           EdgeSlope::downwards         },
        { EdgeSlope::none,              EdgeSlope::upwards,           EdgeSlope::elevated,          EdgeSlope::downwards         },
        { EdgeSlope::downwards,         EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::upwards           },
        { EdgeSlope::downwards,         EdgeSlope::upwards,           EdgeSlope::downwards,         EdgeSlope::upwards           },
        { EdgeSlope::downwards,         EdgeSlope::none,              EdgeSlope::upwards,           EdgeSlope::elevated          },
        { EdgeSlope::downwards,         EdgeSlope::upwards,           EdgeSlope::elevated,          EdgeSlope::elevated          },
        { EdgeSlope::upwards,           EdgeSlope::downwards,         EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::upwards,           EdgeSlope::elevated,          EdgeSlope::downwards,         EdgeSlope::none              },
        { EdgeSlope::upwards,           EdgeSlope::downwards,         EdgeSlope::upwards,           EdgeSlope::downwards         },
        { EdgeSlope::upwards,           EdgeSlope::elevated,          EdgeSlope::elevated,          EdgeSlope::downwards         },
        { EdgeSlope::elevated,          EdgeSlope::downwards,         EdgeSlope::none,              EdgeSlope::upwards           },
        { EdgeSlope::elevated,          EdgeSlope::elevated,          EdgeSlope::downwards,         EdgeSlope::upwards           },
        { EdgeSlope::elevated,          EdgeSlope::downwards,         EdgeSlope::upwards,           EdgeSlope::elevated          },
        { EdgeSlope::elevated,          EdgeSlope::elevated,          EdgeSlope::elevated,          EdgeSlope::elevated          },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::downwards,         EdgeSlope::upwards,           EdgeSlope::upwardsElevated,   EdgeSlope::downwardsElevated },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::upwards,           EdgeSlope::upwardsElevated,   EdgeSlope::downwardsElevated, EdgeSlope::downwards         },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
        { EdgeSlope::upwardsElevated,   EdgeSlope::downwardsElevated, EdgeSlope::downwards,         EdgeSlope::upwards           },
        { EdgeSlope::downwardsElevated, EdgeSlope::downwards,         EdgeSlope::upwards,           EdgeSlope::upwardsElevated   },
        { EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none,              EdgeSlope::none              },
    } };
    // clang-format on

    // 0x004C4A3B
    static bool canConstructWall(World::Pos3 pos, SmallZ baseZ, SmallZ clearZ, uint8_t targetEdge)
    {
        if (!validCoords(pos) || (pos.x < kTileSize || pos.y < kTileSize))
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
            if (clearZ >= el.clearZ())
            {
                continue;
            }
            if (baseZ <= el.baseZ())
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

        auto targetHeight = surface->baseHeight();
        auto edge = args.rotation & 3;
        auto wallFlags = edgeWallMapping[surface->slope()][edge];

        if ((wallFlags & EdgeSlope::elevated) != EdgeSlope::none)
        {
            targetHeight += kSmallZStep;
            wallFlags &= ~EdgeSlope::elevated;
        }

        if (surface->water() && surface->water() * kMicroToSmallZStep > targetHeight)
        {
            setErrorText(StringIds::cant_build_this_underwater);
            return FAILURE;
        }

        if (targetHeight < surface->baseHeight())
        {
            setErrorText(StringIds::error_can_only_build_above_ground);
            return FAILURE;
        }

        if ((wallFlags & (EdgeSlope::upwards | EdgeSlope::downwards)) != EdgeSlope::none)
        {
            auto testHeight = targetHeight + kSmallZStep;

            // Test placement edges to ensure we don't build partially underground
            for (auto i = 2; i <= 3; i++)
            {
                auto testEdge = (args.rotation + i) & 3;
                if (surface->slope() & (1 << testEdge))
                {
                    if (args.pos.z < surface->baseZ())
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
                                if (args.pos.z < testHeight + kSmallZStep)
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

        // TODO: fold into previous block; left for now to match IDA
        auto* wallObj = ObjectManager::get<WallObject>(args.type);
        if ((wallFlags & (EdgeSlope::upwards | EdgeSlope::downwards)) != EdgeSlope::none)
        {
            if ((wallObj->flags & WallObjectFlags::onlyOnLevelLand) != WallObjectFlags::none)
            {
                setErrorText(StringIds::can_only_build_this_on_level_land);
                return FAILURE;
            }

            targetHeight += kSmallZStep;
        }

        auto clearZ = targetHeight + wallObj->var_08;
        if (!canConstructWall(args.pos, targetHeight, clearZ, args.rotation))
        {
            return FAILURE;
        }

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            // Error message set in checkFreeElementsAndReorganise
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        // TODO: actually insert the element 0x004C4594 onwards

        return 0;
    }

    void createWall(registers& regs)
    {
        WallPlacementArgs args(regs);
        regs.ebx = createWall(args, regs.bl);
    }
}
