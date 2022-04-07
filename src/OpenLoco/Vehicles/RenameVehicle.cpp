#include "../Economy/Expenditures.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Localisation/StringManager.h"
#include "../Types.hpp"
#include "Vehicle.h"
#include <array>
#include <unordered_map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<EntityId, 0x0113621D> _113621D;

    /**
     * 0x004B6572 
     * Rename vehicle - calls 3 times with part of the buffers (each 12 bytes long), in each cycle first without apply flag, second with apply flag.
     * The vehicle name is up to 31 characters long (no more characters were taken during the game play).
     *
     * @param flags @<bl> - game command flags
     * @param headId @<cx> - head id of the vehicle
     * @param index @<ax> - update index (in order of: 1, 2, 0)
     * @param buffer0 @<edx> - First group of 4 characters of a 12 character update buffer
     * @param buffer1 @<dx> - Second group of 4 characters of a 12 character update buffer
     * @param buffer2 @<bp> - Third group of 4 characters of a 12 character update buffer
     * @return @<ebx> - if rename is successful, return 0, if failed, return GameCommands::FAILURE
     */
    static uint32_t rename(const uint8_t flags, EntityId headId, int16_t index, uint32_t buffer0, uint32_t buffer1, uint32_t buffer2)
    {
        GameCommands::setExpenditureType(ExpenditureType::TrainRunningCosts);

        static loco_global<EntityId, 0x0113621D> _headId_113621D;
        if (index == 1)
        {
            _headId_113621D = headId;
        }

        static uint32_t staticRenameBuffer[9];

        if ((flags & GameCommands::Flags::apply) != 0)
        {
            static const std::array<int, 3> transformTable = { 2, 0, 1 };
            int arrayIndex = transformTable.at(index);
            staticRenameBuffer[arrayIndex * 3] = buffer0;
            staticRenameBuffer[arrayIndex * 3 + 1] = buffer1;
            staticRenameBuffer[arrayIndex * 3 + 2] = buffer2;
        }

        if (index != 0)
        {
            return 0;
        }

        EntityId vehicleHeadId = _113621D;
        Vehicles::VehicleHead* vehicleHead = EntityManager::get<Vehicles::VehicleHead>(vehicleHeadId);

        if (vehicleHead == nullptr)
        {
            return GameCommands::FAILURE;
        }
        char renameStringBuffer[37] = "";
        memcpy(renameStringBuffer, staticRenameBuffer, sizeof(staticRenameBuffer));
        renameStringBuffer[36] = '\0';

        char existingVehicleName[512];
        auto args = FormatArguments::common(vehicleHead->ordinalNumber);
        StringManager::formatString(existingVehicleName, vehicleHead->name, &args);
        if (strcmp(existingVehicleName, renameStringBuffer) == 0)
        {
            return 0;
        }

        string_id allocatedStringId = StringIds::empty;
        if (strlen(renameStringBuffer) != 0)
        {
            allocatedStringId = StringManager::userStringAllocate(renameStringBuffer, 0);
            if (allocatedStringId == StringIds::empty)
            {
                return GameCommands::FAILURE;
            }
            if ((flags & GameCommands::Flags::apply) == 0)
            {
                StringManager::emptyUserString(allocatedStringId);
                return 0;
            }
        }
        else
        {
            if ((flags & GameCommands::Flags::apply) == 0)
            {
                return 0;
            }

            static const std::unordered_map<VehicleType, string_id> defaultVehicleStringIdMap = {
                { VehicleType::train, StringIds::train_number },
                { VehicleType::bus, StringIds::bus_number },
                { VehicleType::truck, StringIds::truck_number },
                { VehicleType::tram, StringIds::tram_number },
                { VehicleType::aircraft, StringIds::aircraft_number },
                { VehicleType::ship, StringIds::ship_number }
            };
            allocatedStringId = defaultVehicleStringIdMap.at(vehicleHead->vehicleType);
        }

        string_id oldStringId = vehicleHead->name;
        vehicleHead->name = allocatedStringId;
        StringManager::emptyUserString(oldStringId);
        Gfx::invalidateScreen();
        return 0;
    }

    void rename(registers& regs)
    {
        regs.ebx = rename(regs.bl, EntityId(regs.cx), regs.ax, regs.edx, regs.ebp, regs.edi);
    }

}
