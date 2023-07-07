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
    static Town* initialiseTown()
    {
        registers regs;
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

    // 0x00496C22
    static uint32_t createTown(const TownPlacementArgs& args, const uint8_t flags)
    {
        auto& gameState = getGameState();
        Town* newTown = nullptr;

        if (args.pos.x == -1)
        {
            for (auto attempts = 200; attempts > 0; attempts--)
            {
                uint32_t rand = gameState.rng.randNext();
                auto pos = Pos2(((rand & 0xFFFF) * kMapRows) >> 5, ((rand >> 16) * kMapColumns) >> 5);

                if (pos.x < 384 || pos.y < 384 || pos.x > 11904 || pos.y > 11904)
                {
                    continue;
                }

                auto tile = TileManager::get(pos);
                auto* surfaceEl = tile.surface();
                if (surfaceEl->slope() != 0 || surfaceEl->water() != 0)
                {
                    continue;
                }

                auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                if (landObj->hasFlags(LandObjectFlags::isDesert | LandObjectFlags::noTrees))
                {
                    auto dx = sub_4C5604(pos);
                    if (dx < 10 && gameState.rng.randNext() & 0xFF)
                    {
                        continue;
                    }
                }

                // loc_496E89
                for (auto& town : TownManager::towns())
                {
                    auto xDiff = std::abs(pos.x - town.x);
                    auto yDiff = std::abs(pos.y - town.y);
                    if (xDiff + yDiff < 768)
                    {
                        break;
                    }
                }

                newTown = initialiseTown();
                if (!newTown)
                {
                    setErrorText(StringIds::too_many_towns);
                    return FAILURE;
                }
            }
        }
        else
        {
            // TODO: loc_496C32 onwards
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
