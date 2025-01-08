#include "RenameCompanyName.h"
#include "Economy/Expenditures.h"
#include "Engine/Limits.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Gfx.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Types.hpp"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    /**
     * 0x00434914
     * Renames a particular company.
     *
     * This command is called 3 times before the buffer is applied. Each time, 12 chars of the 36 char buffer are provided.
     * The resulting company name has a maximum length of 31 chars; the last bytes are not used.
     */
    static uint32_t changeCompanyName(const ChangeCompanyNameArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Keep track of the company id over several calls.
        static CompanyId _companyId{};
        if (args.bufferIndex == 1)
        {
            _companyId = args.companyId;
        }

        static char renameBuffer[37]{};

        // Fill buffer over calls into the renameBuffer
        if ((flags & GameCommands::Flags::apply) != 0)
        {
            static const std::array<int, 3> transformTable = { 2, 0, 1 };
            const int arrayIndex = transformTable.at(args.bufferIndex);
            std::memcpy(renameBuffer + arrayIndex * 12, args.buffer, 12);
        }

        // Applying the buffer?
        if (args.bufferIndex != 0)
        {
            return 0;
        }

        char renameStringBuffer[37] = "";
        memcpy(renameStringBuffer, renameBuffer, sizeof(renameBuffer));
        renameStringBuffer[36] = '\0';

        // Ensure the new name isn't empty.
        if (strlen(renameStringBuffer) == 0)
        {
            return 0;
        }

        // Retrieve the current name for this company.
        char currentCompanyName[256] = "";
        auto company = CompanyManager::get(_companyId);
        StringManager::formatString(currentCompanyName, company->name);

        // Verify the new name actually differs from the old one.
        if (strcmp(currentCompanyName, renameStringBuffer) == 0)
        {
            return 0;
        }

        // Verify the name isn't in use by any other company
        for (auto& rival : CompanyManager::companies())
        {
            if (rival.id() == _companyId)
            {
                continue;
            }

            char rivalCompanyName[256] = "";
            StringManager::formatString(rivalCompanyName, rival.name);

            if (strcmp(rivalCompanyName, renameStringBuffer) == 0)
            {
                GameCommands::setErrorText(StringIds::chosen_name_in_use);
                return GameCommands::FAILURE;
            }
        }

        // Allocate a string id for the new name.
        StringId allocatedStringId = StringManager::userStringAllocate(renameStringBuffer, false);
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

        // Apply the new name to the company.
        StringId oldStringId = company->name;
        company->name = allocatedStringId;
        StringManager::emptyUserString(oldStringId);
        Gfx::invalidateScreen();
        return 0;
    }

    void changeCompanyName(registers& regs)
    {
        regs.ebx = changeCompanyName(ChangeCompanyNameArgs(regs), regs.bl);
    }
}
