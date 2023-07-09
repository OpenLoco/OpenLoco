#include "Audio/Audio.h"
#include "Date.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "GameState.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/SurfaceElement.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    // 0x00496FE7
    // TODO: only used here, but move to town manager?
    static Town* initialiseTown(Pos2 pos)
    {
        registers regs;
        regs.eax = pos.x;
        regs.ecx = pos.y;
        call(0x00496FE7, regs);

        if (regs.esi != -1)
            return reinterpret_cast<Town*>(regs.esi);
        else
            return nullptr;
    }

    // 0x00496E09
    static bool checkSurroundings(Pos2 pos, bool checkSurroundingWater)
    {
        auto tile = TileManager::get(pos);
        auto* surfaceEl = tile.surface();
        if (surfaceEl->slope() != 0 || surfaceEl->water() != 0)
        {
            return false;
        }

        if (checkSurroundingWater)
        {
            auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
            if (landObj->hasFlags(LandObjectFlags::isDesert | LandObjectFlags::noTrees))
            {
                auto nearbyWaterTiles = TileManager::countNearbyWaterTiles(pos);
                if (nearbyWaterTiles < 10 && getGameState().rng.randNext() & 0xFF)
                {
                    return false;
                }
            }
        }

        for (auto& town : TownManager::towns())
        {
            auto manhattanDistance = Math::Vector::manhattanDistance(pos, Pos2(town.x, town.y));
            if (manhattanDistance < 768)
            {
                setErrorText(StringIds::too_close_to_another_town);
                return false;
            }
        }

        return true;
    }

    // 0x00496C22
    static uint32_t createTown(const TownPlacementArgs& args, const uint8_t flags)
    {
        auto& gameState = getGameState();
        Pos2 pos = args.pos;
        Town* newTown = nullptr;

        if (args.pos.x == -1)
        {
            bool foundPos = false;
            for (auto attempts = 200; attempts > 0; attempts--)
            {
                uint32_t rand = gameState.rng.randNext();
                auto tilePos = TilePos2(((rand >> 16) * kMapColumns) >> 16, ((rand & 0xFFFF) * kMapRows) >> 16);
                Pos2 attemptPos = toWorldSpace(tilePos);

                if (attemptPos.x < 384 || attemptPos.y < 384 || attemptPos.x > 11904 || attemptPos.y > 11904)
                {
                    continue;
                }

                if (checkSurroundings(attemptPos, true))
                {
                    pos = attemptPos;
                    foundPos = true;
                    break;
                }
            }

            if (!foundPos)
            {
                return FAILURE;
            }
        }
        else
        {
            bool foundPos = false;
            for (int attempts = 0; attempts < 40; attempts++)
            {
                Pos2 attemptPos = pos;
                if (attempts <= 10)
                {
                    // Add random value [-2, 1] to x and y
                    uint32_t rand = gameState.rng.randNext();
                    TilePos2 randOffset(((rand >> 9) & 3) - 2, ((rand >> 5) & 3) - 2);
                    attemptPos = pos + World::toWorldSpace(randOffset);
                }
                else if (attempts > 10)
                {
                    // Add random value [-3, 4] to x and y
                    uint32_t rand = gameState.rng.randNext();
                    TilePos2 randOffset(((rand >> 9) & 7) - 3, ((rand >> 5) & 7) - 3);
                    attemptPos = pos + World::toWorldSpace(randOffset);
                }

                if (attemptPos.x < 160 || attemptPos.y < 160 || attemptPos.x > 12128 || attemptPos.y > 12128)
                {
                    continue;
                }

                if (checkSurroundings(attemptPos, false))
                {
                    pos = attemptPos;
                    foundPos = true;
                    break;
                }
            }

            if (!foundPos)
            {
                return FAILURE;
            }
        }

        newTown = initialiseTown(pos);
        if (!newTown)
        {
            setErrorText(StringIds::too_many_towns);
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            StringManager::emptyUserString(newTown->name);
            newTown->name = StringIds::null;
            return 0;
        }

        // Backup current year, and adjust temporarily
        auto backupYear = getCurrentYear();
        setCurrentYear(backupYear - 51);

        auto growthFactor = args.size * args.size;

        for (int i = 8; i > 0; i--)
        {
            for (int j = growthFactor; j > 0; j--)
            {
                newTown->grow(0xFF);
                newTown->recalculateSize();
            }

            setCurrentYear(getCurrentYear() + 7);
        }

        // Restore current year
        setCurrentYear(backupYear);

        newTown->history[newTown->historySize - 1] = std::max<uint8_t>(newTown->population / 50, 255);

        auto tileHeight = World::TileManager::getHeight(Pos2(newTown->x, newTown->y));
        setPosition(World::Pos3(newTown->x + World::kTileSize / 2, newTown->y + World::kTileSize / 2, tileHeight.landHeight));

        auto& options = S5::getOptions();
        options.madeAnyChanges = 1;

        return 0;
    }

    void createTown(registers& regs)
    {
        TownPlacementArgs args(regs);
        regs.ebx = createTown(args, regs.bl);
    }
}
