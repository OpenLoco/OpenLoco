#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/QuarterTile.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
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

        auto* surfaceEl = World::TileManager::get(args.pos).surface();
        if (surfaceEl == nullptr)
        {
            return FAILURE;
        }

        auto targetHeight = surface->baseHeight();
        auto edge = args.rotation & 3;
        auto wallFlags = edgeWallMapping[surface->slope()][edge];

        if (wallFlags & (1 << 0))
        {
            targetHeight += kSmallZStep;
            wallFlags &= ~(1 << 0);
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

        if (wallFlags & 0x90)
        {
            // to 0x004C463F
        }

        // continue from 0x004C44A1

        return 0;
    }

    void createWall(registers& regs)
    {
        WallPlacementArgs args(regs);
        regs.ebx = createWall(args, regs.bl);
    }
}
