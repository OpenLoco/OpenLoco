#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    void changeCompanyFace(registers& regs);

    struct ChangeCompanyFaceArgs
    {
        static constexpr auto command = GameCommand::changeCompanyFace;

        ChangeCompanyFaceArgs() = default;
        explicit ChangeCompanyFaceArgs(const registers& regs)
            : companyId(CompanyId(regs.bh))
            , objHeader()
        {
            uint8_t objData[sizeof(ObjectHeader)]{};

            uint8_t* objPtr = objData;
            std::memcpy(objPtr, &regs.eax, sizeof(uint32_t));
            objPtr += sizeof(uint32_t);
            std::memcpy(objPtr, &regs.ecx, sizeof(uint32_t));
            objPtr += sizeof(uint32_t);
            std::memcpy(objPtr, &regs.edx, sizeof(uint32_t));
            objPtr += sizeof(uint32_t);
            std::memcpy(objPtr, &regs.edi, sizeof(uint32_t));

            std::memcpy(&objHeader, objData, sizeof(objData));
        }

        CompanyId companyId;
        ObjectHeader objHeader;

        explicit operator registers() const
        {
            registers regs;
            regs.bh = enumValue(companyId); // company id
            uint8_t objData[sizeof(ObjectHeader)]{};
            std::memcpy(objData, &objHeader, sizeof(objData));

            const uint8_t* objPtr = objData;
            std::memcpy(&regs.eax, objPtr, sizeof(uint32_t));
            objPtr += sizeof(uint32_t);
            std::memcpy(&regs.ecx, objPtr, sizeof(uint32_t));
            objPtr += sizeof(uint32_t);
            std::memcpy(&regs.edx, objPtr, sizeof(uint32_t));
            objPtr += sizeof(uint32_t);
            std::memcpy(&regs.edi, objPtr, sizeof(uint32_t));

            return regs;
        }
    };
}
