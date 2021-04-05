#include "../Industry.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Localisation/StringManager.h"
#include "../Management/Expenditures.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Types.hpp"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<uint8_t, 0x009C68EA> gGameCommandExpenditureType; // premultiplied by 4

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
    static uint32_t renameTown(const uint8_t flags, TownId_t townId, int16_t index, uint32_t buffer0, uint32_t buffer1, uint32_t buffer2)
    {
        gGameCommandExpenditureType = static_cast<uint8_t>(ExpenditureType::Miscellaneous) * 4;

        // Keep track of the town id over several calls.
        static TownId_t _townId{};
        if (index == 1)
            _townId = townId;

        static uint32_t renameBuffer[9];

        // Fill buffer over calls into the renameBuffer
        if ((flags & GameCommands::apply) != 0)
        {
            static const std::array<int, 3> transformTable = { 2, 0, 1 };
            int arrayIndex = transformTable.at(index);
            renameBuffer[arrayIndex * 3] = buffer0;
            renameBuffer[arrayIndex * 3 + 1] = buffer1;
            renameBuffer[arrayIndex * 3 + 2] = buffer2;
        }

        // Applying the buffer?
        if (index != 0)
            return 0;

        char renameStringBuffer[37] = "";
        memcpy(renameStringBuffer, renameBuffer, sizeof(renameStringBuffer));
        renameStringBuffer[36] = '\0';

        // Ensure the new name isn't empty.
        if (strlen(renameStringBuffer) == 0)
            return 0;

        // Figure out the current name for this town.
        char currentTownName[256] = "";
        auto town = TownManager::get(_townId);
        StringManager::formatString(currentTownName, town->name);

        // Verify the new name actually differs from the old one.
        if (strcmp(currentTownName, renameStringBuffer) == 0)
            return 0;

        // Allocate a string id for the new name.
        string_id allocatedStringId = StringManager::userStringAllocate(renameStringBuffer, 0);
        if (allocatedStringId == StringIds::empty)
            return GameCommands::FAILURE;

        // Bailing out early?
        if ((flags & GameCommands::apply) == 0)
        {
            StringManager::emptyUserString(allocatedStringId);
            return 0;
        }

        // Apply the new name to the town.
        string_id oldStringId = town->name;
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
        regs.ebx = renameTown(regs.bl, regs.cx, regs.ax, regs.edx, regs.ebp, regs.edi);
    }
}
