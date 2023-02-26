#include "Audio/Audio.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "ViewportManager.h"
#include "World/TownManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<Core::Prng, 0x00525E20> _prng;

    // 0x0048B089
    static void playDemolishTreeSound(const World::Pos3& loc)
    {
        const auto frequency = _prng->randNext(20003, 24098);
        Audio::playSound(Audio::SoundId::demolishTree, loc, -1100, frequency);
    }

    // 0x004BB432
    // bl = flags;
    // esi = X86Pointer(&element);
    // ax = pos.x;
    // cx = pos.y;
    // TODO: Move to somewhere else multiple functions call this one
    static void removeTree(World::TreeElement& element, const uint8_t flags, const World::Pos2& pos)
    {
        if ((!element.isGhost() && !element.isFlag5())
            && getUpdatingCompanyId() != CompanyId::neutral)
        {
            auto loc = World::Pos3(pos.x, pos.y, element.baseHeight());
            playDemolishTreeSound(loc);
        }

        if ((flags & Flags::flag_6) == 0)
        {
            auto treeObj = ObjectManager::get<TreeObject>(element.treeObjectId());
            auto ratingReduction = treeObj->demolishRatingReduction;
            TownManager::updateTownInfo(pos, 0, 0, ratingReduction, 0);
        }

        auto zMin = element.baseHeight();
        auto zMax = element.clearHeight();
        Ui::ViewportManager::invalidate(pos, zMin, zMax, ZoomLevel::eighth, 56);

        World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(&element));
    }

    /**
     * 0x004BB392
     * Remove tree
     *
     * This is called when you activate the Plant Trees from the construction (first) menu and you move the cursor over the terrain.
     *
     * @param pos_x @<ax>
     * @param pos_y @<cx>
     * @param pos_z @<dl> * World::kSmallZStep
     * @param type @<dh>
     * @param elementType @<bh>
     * @param flags @<bl>
     * @return @<ebx> - returns the remove cost if successful; otherwise GameCommands::FAILURE (in the assembly code we never get into failure path)
     */
    static uint32_t removeTree(const World::Pos3& pos, const uint8_t type, const uint8_t elementType, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(pos);
        GameCommands::setPosition(World::Pos3(pos.x + World::kTileSize / 2, pos.y + World::kTileSize / 2, tileHeight.landHeight));

        auto tile = World::TileManager::get(pos);
        for (auto& element : tile)
        {
            // TODO: refactor! Figure out what info it actually needs.
            if (element.rawData()[0] != elementType)
                continue;

            if (element.baseHeight() != pos.z)
                continue;

            auto* treeElement = element.as<World::TreeElement>();
            if (treeElement == nullptr)
                continue;

            if (treeElement->treeObjectId() != type)
                continue;

            auto treeObj = ObjectManager::get<TreeObject>(treeElement->treeObjectId());
            currency32_t removalCost = Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);

            if (flags & Flags::apply)
                removeTree(*treeElement, flags, pos);

            auto& options = S5::getOptions();
            options.madeAnyChanges = 1;

            return removalCost;
        }

        return FAILURE;
    }

    void removeTree(registers& regs)
    {
        TreeRemovalArgs args(regs);
        regs.ebx = removeTree(args.pos, args.type, args.elementType, regs.bl);
    }
}
