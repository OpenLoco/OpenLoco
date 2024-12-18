#include "RenameCompanyOwner.h"
#include "ChangeCompanyFace.h"
#include "Economy/Expenditures.h"
#include "Engine/Limits.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Gfx.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Types.hpp"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    void tryLoadInstalledFaceForName(const CompanyId id, const char* companyName);

    /**
     * 0x00434914
     * Renames a particular company owner.
     *
     * This command is called 3 times before the buffer is applied. Each time, 12 chars of the 36 char buffer are provided.
     * The resulting company owner name has a maximum length of 31 chars; the last bytes are not used.
     */
    static uint32_t changeCompanyOwnerName(const ChangeCompanyOwnerNameArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Keep track of the company id over several calls.
        static CompanyId _companyId{};
        if (args.bufferIndex == 1)
        {
            _companyId = args.companyId;
        }

        static char staticRenameBuffer[37]{};

        // Fill buffer over calls into the renameBuffer
        if ((flags & GameCommands::Flags::apply) != 0)
        {
            static const std::array<int, 3> transformTable = { 2, 0, 1 };
            int arrayIndex = transformTable.at(args.bufferIndex);
            std::memcpy(staticRenameBuffer + arrayIndex * 12, args.newName, 12);
        }

        // Applying the buffer?
        if (args.bufferIndex != 0)
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

        // Retrieve the current name for this company.
        char currentOwnerName[256] = "";
        auto company = CompanyManager::get(_companyId);
        StringManager::formatString(currentOwnerName, company->ownerName);

        // Verify the new name actually differs from the old one.
        if (strcmp(currentOwnerName, renameStringBuffer) == 0)
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

            char rivalOwnerName[256] = "";
            StringManager::formatString(rivalOwnerName, rival.ownerName);

            if (strcmp(rivalOwnerName, renameStringBuffer) == 0)
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

        // Apply the new owner name to the company.
        StringId oldStringId = company->ownerName;
        company->ownerName = allocatedStringId;
        StringManager::emptyUserString(oldStringId);
        Gfx::invalidateScreen();
        tryLoadInstalledFaceForName(_companyId, renameStringBuffer);
        return 0;
    }

    void changeCompanyOwnerName(registers& regs)
    {
        regs.ebx = changeCompanyOwnerName(ChangeCompanyOwnerNameArgs(regs), regs.bl);
    }

    // 0x00434BA1
    void tryLoadInstalledFaceForName(const CompanyId id, const char* companyName)
    {
        const auto competitorInstalledObjects = CompanyManager::findAllOtherInUseCompetitors(id);

        std::optional<ObjectHeader> foundInstalledObject = std::nullopt;
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
        {
            auto res = std::find(competitorInstalledObjects.begin(), competitorInstalledObjects.end(), object.index);
            if (res != competitorInstalledObjects.end())
            {
                continue;
            }

            // Copy the string as it needs some processing
            std::string objectName = object.object._name;
            // Not sure what ControlCodes::pop16 is doing in the object name but it is at the start of all the object names
            objectName.erase(std::remove(std::begin(objectName), std::end(objectName), static_cast<char>(ControlCodes::pop16)));

            auto strcmpSpecial = [](const char* lhs, const char* rhs) {
                while (*lhs && *rhs)
                {
                    const auto chr = *lhs;
                    if (chr == '\xFF')
                    {
                        if (chr != *rhs)
                        {
                            return false;
                        }
                        lhs++;
                        rhs++;
                        const uint16_t unkLhs = *reinterpret_cast<const uint16_t*>(lhs);
                        const uint16_t unkRhs = *reinterpret_cast<const uint16_t*>(rhs);
                        if (unkLhs != unkRhs)
                        {
                            return false;
                        }
                        lhs += 2;
                        rhs += 2;
                        continue;
                    }
                    auto toUpper = [](const char chr) -> char {
                        if (chr < 'a' || chr > 'z')
                        {
                            return chr;
                        }
                        return chr - 0x20;
                    };
                    if (toUpper(chr) != toUpper(*rhs))
                    {
                        return false;
                    }
                    lhs++;
                    rhs++;
                }
                return *lhs == *rhs;
            };

            if (strcmpSpecial(companyName, objectName.c_str()))
            {
                foundInstalledObject = object.object._header;
                break;
            }
        }

        if (foundInstalledObject.has_value())
        {
            ChangeCompanyFaceArgs args{};
            args.companyId = id;
            args.objHeader = foundInstalledObject.value();

            auto regs = registers(args);
            regs.bl = Flags::apply;
            changeCompanyFace(regs);
        }
    }
}
