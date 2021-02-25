#include "../GameCommands.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringManager.h"
#include "../Management/Expenditures.h"
#include "../Things/ThingManager.h"
#include "../Types.hpp"
#include "Vehicle.h"
#include <array>

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<uint8_t, 0x009C68EA> gGameCommandExpenditureType; // premultiplied by 4
    static loco_global<uint32_t[9], 0x011361F8> gRenameBuffer;
    static loco_global<char[36], 0x011361F8> gRenameBufferAsString;
    static loco_global<uint16_t, 0x0113621D> _113621D;

    // 0x004B6572
    static uint32_t rename(const uint8_t flags /* bl */, thing_id_t headId /* cx */, int16_t index /* ax */, uint32_t buffer0 /* edx */, uint32_t buffer1 /* ebp */, uint32_t buffer2 /* edi */)
    {
        static loco_global<thing_id_t, 0x0113621D> _headId_113621D;
        gGameCommandExpenditureType = static_cast<uint8_t>(ExpenditureType::TrainRunningCosts) * 4; // 004B6572
        if (index == 1)                                                                             // 004B6579-004B657D
        {
            _headId_113621D = headId; // 004B657F
        }

        if ((flags & GameCommands::apply) != 0) // 004B6586-004B6589
        {
            static const std::array<int, 3> transformTable = { 2, 0, 1 };
            int arrayIndex = transformTable.at(index);   // 004B658B-004B6591
            gRenameBuffer[arrayIndex * 3] = buffer0;     // 004B6596-004B6599
            gRenameBuffer[arrayIndex * 3 + 1] = buffer1; // 004B659F
            gRenameBuffer[arrayIndex * 3 + 2] = buffer2; // 004B65A5
        }

        if (index != 0) // 004B65AB
        {
            return 0; // 004B6655-004B6657
        }

        thing_id_t vehicleHeadId = _113621D;                                                          // 004B65B6
        Vehicles::VehicleHead* vehicleHead = ThingManager::get<Vehicles::VehicleHead>(vehicleHeadId); // 004B65BD-004B65C0

        static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;

        auto args = FormatArguments::common(vehicleHead->var_44);                     // 004B65C6-004B65CA
        StringManager::formatString(_stringFormatBuffer, vehicleHead->var_22, &args); // 004B65D0-004B65E6
        if (strcmp(_stringFormatBuffer, gRenameBufferAsString) == 0)                  // 004B65F1-004B6601
        {
            return 0; // 004B6655-004B6657
        }

        uint16_t allocatedStringId = 0;
        if (strlen(gRenameBufferAsString) != 0) // 004B6603-004B660A
        {
            allocatedStringId = StringManager::userStringAllocate(gRenameBufferAsString, 0); // 004B660C-004B6613
            if (allocatedStringId == 0)                                                      // 004B6618-004B661B
            {
                return GameCommands::FAILURE; // 004B664D-004B6654
            }
            if ((flags & GameCommands::apply) == 0) // 004B661D-004B6620
            {
                StringManager::emptyUserString(allocatedStringId); // 004B6634-004B663B, 004B6655-004B6657
                return 0;
            }
        }
        else
        {
            if ((flags & GameCommands::apply) == 0) // 004B663D-004B6640
            {
                return 0; // 004B6639-004B663B, 004B6655-004B6657
            }
            allocatedStringId = static_cast<uint16_t>(vehicleHead->vehicleType) + 4; // 004B6642-004B664B
        }

        uint16_t oldStringId = vehicleHead->var_22;
        vehicleHead->var_22 = allocatedStringId;     // 004B6622
        StringManager::emptyUserString(oldStringId); // 004B6626
        Gfx::invalidateScreen();                     // 004B662B
        return 0;                                    // 004B6630-004B6632, 004B6655-004B6657

        //registers regs;
        //regs.ax = index;
        //regs.bl = flags;
        //regs.cx = headId;
        //regs.edx = buffer0;
        //regs.ebp = buffer1;
        //regs.edi = buffer2;
        //call(0x004B6572, regs);
        //return regs.ebx;
    }

    void rename(registers& regs)
    {
        regs.ebx = rename(regs.bl, regs.cx, regs.ax, regs.edx, regs.ebp, regs.edi);
    }

}
