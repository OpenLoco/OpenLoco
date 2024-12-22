#include "RenameIndustry.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Industries/RenameIndustry.h"
#include "Graphics/Gfx.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Types.hpp"
#include "World/Industry.h"
#include "World/IndustryManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    /**
     * 0x00455029
     * Renames a particular industry.
     *
     * This command is called 3 times before the buffer is applied. Each time, 12 chars of the 36 char buffer are provided.
     * The resulting industry name has a maximum length of 31 chars; the last bytes are not used.
     *
     * @param flags @<bl> - game command flags
     * @param industryId @<cl> - industry id
     * @param index @<ax> - update index (in order of: 1, 2, 0)
     * @param buffer0 @<edx> - First part (4 chars) of the 12 update buffer
     * @param buffer1 @<dx> - Second part (4 chars) of the 12 update buffer
     * @param buffer2 @<bp> - Third part (4 chars) of the 12 update buffer
     * @return @<ebx> - returns 0 if rename is successful; otherwise GameCommands::FAILURE
     */
    static uint32_t renameIndustry(const GameCommands::RenameIndustryArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Keep track of the industry id over several calls.
        static IndustryId _industryId{};
        if (args.nameBufferIndex == 1)
        {
            _industryId = args.industryId;
        }

        static char staticRenameBuffer[37]{};

        // Fill buffer over calls into the renameBuffer
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

        // Figure out the current name for this industry.
        char currentIndustryName[256] = "";
        auto industry = IndustryManager::get(_industryId);
        FormatArguments fargs{};
        fargs.push(industry->town);
        StringManager::formatString(currentIndustryName, industry->name, fargs);

        // Verify the new name actually differs from the old one.
        if (strcmp(currentIndustryName, renameStringBuffer) == 0)
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

        // Apply the new name to the industry.
        StringId oldStringId = industry->name;
        industry->name = allocatedStringId;
        StringManager::emptyUserString(oldStringId);
        Gfx::invalidateScreen();
        return 0;
    }

    void renameIndustry(registers& regs)
    {
        regs.ebx = renameIndustry(GameCommands::RenameIndustryArgs(regs), regs.bl);
    }
}
