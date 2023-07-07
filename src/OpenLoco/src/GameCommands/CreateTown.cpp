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

    // 0x004C5604
    // TODO: used by other functions; move elsewhere
    static uint16_t sub_4C5604(Pos2 pos)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004C5604, regs);
        return regs.dx;
    }

    static bool checkSurroundings(Pos2 pos)
    {
        auto tile = TileManager::get(pos);
        auto* surfaceEl = tile.surface();
        if (surfaceEl->slope() != 0 || surfaceEl->water() != 0)
        {
            printf("Slope or water present -- bailing out\n");
            return false;
        }

        auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
        if (landObj->hasFlags(LandObjectFlags::isDesert | LandObjectFlags::noTrees))
        {
            auto dx = sub_4C5604(pos);
            if (dx < 10 && getGameState().rng.randNext() & 0xFF)
            {
                printf("sub_4C5604 does not pass -- bailing out\n");
                return false;
            }
        }

        for (auto& town : TownManager::towns())
        {
            auto manhattanDistance = Math::Vector::manhattanDistance(pos, Pos2(town.x, town.y));
            if (manhattanDistance < 768)
            {
                printf("manhattanDistance passes, yay\n");
                return true;
            }
        }

        printf("manhattanDistance did not pass, yay\n");
        return false;
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
                pos = Pos2(((rand & 0xFFFF) * kMapRows) >> 5, ((rand >> 16) * kMapColumns) >> 5);

                if (pos.x < 384 || pos.y < 384 || pos.x > 11904 || pos.y > 11904)
                {
                    printf("Invalid position -- trying again\n");
                    continue;
                }

                if (checkSurroundings(pos))
                {
                    printf("Surroundings pass the sniff test -- success\n");
                    foundPos = true;
                    break;
                }
            }

            if (!foundPos)
            {
                printf("Unable to find suitable surroundings (-1) -- bailing out\n");
                return FAILURE;
            }
        }
        else
        {
            bool foundPos = false;
            for (int attempts = 0; attempts < 40; attempts++)
            {
                if (attempts == 0)
                {
                    uint32_t rand = gameState.rng.randNext();
                    pos += Pos2((rand & 0xE0) - 0x60, ((rand >> 4) & 0xE0) - 0x60);
                }
                else if (attempts <= 10)
                {
                    uint32_t rand = gameState.rng.randNext();
                    pos += Pos2((rand & 0x60) - kTileSize, ((rand >> 4) & 0x60) - kTileSize);
                }

                if (pos.x < 160 || pos.y < 160 || pos.x > 12128 || pos.y > 12128)
                {
                    printf("Invalid position -- trying again\n");
                    continue;
                }

                if (checkSurroundings(pos))
                {
                    printf("Surroundings pass the sniff test -- success\n");
                    foundPos = true;
                    break;
                }
            }

            if (!foundPos)
            {
                printf("Unable to find suitable surroundings (-2) -- bailing out\n");
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

        auto currentYear = getCurrentYear();
        setCurrentYear(currentYear - 51);

        auto growthFactor = args.size * args.size;

        for (int i = 8; i > 0; i--)
        {
            for (int j = growthFactor; j > 0; j--)
            {
                newTown->grow(0xFF);
                newTown->recalculateSize();
            }

            setCurrentYear(currentYear + 7);
        }

        setCurrentYear(currentYear);

        newTown->history[newTown->historySize - 1] = newTown->population / 50;

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
