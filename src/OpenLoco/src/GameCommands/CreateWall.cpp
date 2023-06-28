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

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // clang-format off
    constexpr std::array<std::array<uint8_t, 4>, 32> edgeWallMapping = {
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x40, 0x80, 0x00 },
        { 0x00, 0x00, 0x40, 0x80 },
        { 0x00, 0x40, 0x01, 0x80 },
        { 0x80, 0x00, 0x00, 0x40 },
        { 0x80, 0x40, 0x80, 0x40 },
        { 0x80, 0x00, 0x40, 0x01 },
        { 0x80, 0x40, 0x01, 0x01 },
        { 0x40, 0x80, 0x00, 0x00 },
        { 0x40, 0x01, 0x80, 0x00 },
        { 0x40, 0x80, 0x40, 0x80 },
        { 0x40, 0x01, 0x01, 0x80 },
        { 0x01, 0x80, 0x00, 0x40 },
        { 0x01, 0x01, 0x80, 0x40 },
        { 0x01, 0x80, 0x40, 0x01 },
        { 0x01, 0x01, 0x01, 0x01 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x80, 0x40, 0x41, 0x81 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x40, 0x41, 0x81, 0x80 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x41, 0x81, 0x80, 0x40 },
        { 0x81, 0x80, 0x40, 0x41 },
        { 0x00, 0x00, 0x00, 0x00 },
    };
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
