#include "CompanyManager.h"
#include "Economy/Expenditures.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Limits.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Types.hpp"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    /**
     * 0x00434914
     * Renames a particular company.
     *
     * This command is called 3 times before the buffer is applied. Each time, 12 chars of the 36 char buffer are provided.
     * The resulting company name has a maximum length of 31 chars; the last bytes are not used.
     *
     * @param flags @<bl> - game command flags
     * @param companyId @<cl> - company id
     * @param index @<ax> - update index (in order of: 1, 2, 0)
     * @param buffer0 @<edx> - First part (4 chars) of the 12 update buffer
     * @param buffer1 @<dx> - Second part (4 chars) of the 12 update buffer
     * @param buffer2 @<bp> - Third part (4 chars) of the 12 update buffer
     * @return @<ebx> - returns 0 if rename is successful; otherwise GameCommands::FAILURE
     */
    static uint32_t changeCompanyName(const uint8_t flags, CompanyId companyId, int16_t index, uint32_t buffer0, uint32_t buffer1, uint32_t buffer2)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Keep track of the company id over several calls.
        static CompanyId _companyId{};
        if (index == 1)
            _companyId = companyId;

        static uint32_t renameBuffer[9];

        // Fill buffer over calls into the renameBuffer
        if ((flags & GameCommands::Flags::apply) != 0)
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
        memcpy(renameStringBuffer, renameBuffer, sizeof(renameBuffer));
        renameStringBuffer[36] = '\0';

        // Ensure the new name isn't empty.
        if (strlen(renameStringBuffer) == 0)
            return 0;

        // Retrieve the current name for this company.
        char currentCompanyName[256] = "";
        auto company = CompanyManager::get(_companyId);
        StringManager::formatString(currentCompanyName, company->name);

        // Verify the new name actually differs from the old one.
        if (strcmp(currentCompanyName, renameStringBuffer) == 0)
            return 0;

        // Verify the name isn't in use by any other company
        for (auto& rival : CompanyManager::companies())
        {
            if (rival.id() == _companyId)
                continue;

            char rivalCompanyName[256] = "";
            StringManager::formatString(rivalCompanyName, rival.name);

            if (strcmp(rivalCompanyName, renameStringBuffer) == 0)
            {
                GameCommands::setErrorText(StringIds::chosen_name_in_use);
                return GameCommands::FAILURE;
            }
        }

        // Allocate a string id for the new name.
        string_id allocatedStringId = StringManager::userStringAllocate(renameStringBuffer, 0);
        if (allocatedStringId == StringIds::empty)
            return GameCommands::FAILURE;

        // Bailing out early?
        if ((flags & GameCommands::Flags::apply) == 0)
        {
            StringManager::emptyUserString(allocatedStringId);
            return 0;
        }

        // Apply the new name to the company.
        string_id oldStringId = company->name;
        company->name = allocatedStringId;
        StringManager::emptyUserString(oldStringId);
        Gfx::invalidateScreen();
        return 0;
    }

    void changeCompanyName(registers& regs)
    {
        regs.ebx = changeCompanyName(regs.bl, CompanyId(regs.cl), regs.ax, regs.edx, regs.ebp, regs.edi);
    }
}
