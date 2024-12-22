#include "RenameTown.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Town/RenameTown.h"
#include "Graphics/Gfx.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Types.hpp"
#include "World/Industry.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    /**
     * 0x00455029
     * Renames a particular town.
     *
     * This command is called 3 times before the buffer is applied. Each time, 12 chars of the 36 char buffer are provided.
     * The resulting town name has a maximum length of 31 chars; the last bytes are not used.
     *
     * @param flags @<bl> - game command flags
     * @param townId @<cx> - town id
     * @param index @<ax> - update index (in order of: 1, 2, 0)
     * @param buffer0 @<edx> - First part (4 chars) of the 12 update buffer
     * @param buffer1 @<dx> - Second part (4 chars) of the 12 update buffer
     * @param buffer2 @<bp> - Third part (4 chars) of the 12 update buffer
     * @return @<ebx> - returns 0 if rename is successful; otherwise GameCommands::FAILURE
     */
    static uint32_t renameTown(const GameCommands::RenameTownArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Keep track of the town id over several calls.
        static TownId _townId{};
        if (args.nameBufferIndex == 1)
        {
            _townId = args.townId;
        }

        static char staticRenameBuffer[37]{};

        // Fill buffer over calls into the staticRenameBuffer
        if ((flags & GameCommands::Flags::apply) != 0)
        {
            static constexpr std::array<int, 3> kTransformTable = { 2, 0, 1 };
            int arrayIndex = kTransformTable.at(args.nameBufferIndex);
            std::memcpy(staticRenameBuffer + arrayIndex * 12, args.buffer, 12);
        }

        // Applying the buffer?
        if (args.nameBufferIndex != 0)
        {
            return 0;
        }

        char renameStringBuffer[37] = "";
        memcpy(renameStringBuffer, staticRenameBuffer, sizeof(staticRenameBuffer));
        renameStringBuffer[36] = '\0';

        // Ensure the new name isn't empty.
        if (strlen(renameStringBuffer) == 0)
        {
            return 0;
        }

        // Figure out the current name for this town.
        char currentTownName[256] = "";
        auto town = TownManager::get(_townId);
        StringManager::formatString(currentTownName, town->name);

        // Verify the new name actually differs from the old one.
        if (strcmp(currentTownName, renameStringBuffer) == 0)
        {
            return 0;
        }

        // Allocate a string id for the new name.
        StringId allocatedStringId = StringManager::userStringAllocate(renameStringBuffer, true);
        if (allocatedStringId == StringIds::empty)
        {
            return GameCommands::FAILURE;
        }

        // Bailing out early?
        if ((flags & GameCommands::Flags::apply) == 0)
        {
            StringManager::emptyUserString(allocatedStringId);
            return 0;
        }

        // Apply the new name to the town.
        StringId oldStringId = town->name;
        town->name = allocatedStringId;
        StringManager::emptyUserString(oldStringId);

        // Recalculate labels for the town and (surrounding) stations.
        town->updateLabel();
        StationManager::updateLabels();
        Gfx::invalidateScreen();

        return 0;
    }

    void renameTown(registers& regs)
    {
        regs.ebx = renameTown(GameCommands::RenameTownArgs(regs), regs.bl);
    }
}
