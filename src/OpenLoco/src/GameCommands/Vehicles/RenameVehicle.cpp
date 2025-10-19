#include "RenameVehicle.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <array>
#include <unordered_map>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
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
     * @return @<ebx> - if rename is successful, return 0, if failed, return FAILURE
     */
    static uint32_t renameVehicle(const VehicleRenameArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        static loco_global<EntityId, 0x0113621D> _headId_113621D;
        if (args.i == 1)
        {
            _headId_113621D = args.head;
        }

        static char staticRenameBuffer[37]{};

        if ((flags & Flags::apply) != 0)
        {
            static constexpr std::array<int, 3> kTransformTable = { 2, 0, 1 };
            int arrayIndex = kTransformTable.at(args.i);
            std::memcpy(staticRenameBuffer + arrayIndex * 12, args.buffer, 12);
        }

        if (args.i != 0)
        {
            return 0;
        }

        EntityId vehicleHeadId = _headId_113621D;
        Vehicles::VehicleHead* vehicleHead = EntityManager::get<Vehicles::VehicleHead>(vehicleHeadId);

        if (vehicleHead == nullptr)
        {
            return FAILURE;
        }
        char renameStringBuffer[37] = "";
        memcpy(renameStringBuffer, staticRenameBuffer, sizeof(staticRenameBuffer));
        renameStringBuffer[36] = '\0';

        char existingVehicleName[512];
        FormatArguments fArgs{};
        fArgs.push(vehicleHead->ordinalNumber);
        StringManager::formatString(existingVehicleName, vehicleHead->name, fArgs);

        if (strcmp(existingVehicleName, renameStringBuffer) == 0)
        {
            return 0;
        }

        StringId allocatedStringId = StringIds::empty;
        if (strlen(renameStringBuffer) != 0)
        {
            allocatedStringId = StringManager::userStringAllocate(renameStringBuffer, true);
            if (allocatedStringId == StringIds::empty)
            {
                return FAILURE;
            }
            if ((flags & Flags::apply) == 0)
            {
                StringManager::emptyUserString(allocatedStringId);
                return 0;
            }
        }
        else
        {
            if ((flags & Flags::apply) == 0)
            {
                return 0;
            }

            static const std::unordered_map<VehicleType, StringId> defaultVehicleStringIdMap = {
                { VehicleType::train, StringIds::train_number },
                { VehicleType::bus, StringIds::bus_number },
                { VehicleType::truck, StringIds::truck_number },
                { VehicleType::tram, StringIds::tram_number },
                { VehicleType::aircraft, StringIds::aircraft_number },
                { VehicleType::ship, StringIds::ship_number }
            };
            allocatedStringId = defaultVehicleStringIdMap.at(vehicleHead->vehicleType);
        }

        StringId oldStringId = vehicleHead->name;
        vehicleHead->name = allocatedStringId;
        StringManager::emptyUserString(oldStringId);
        Gfx::invalidateScreen();
        return 0;
    }

    void renameVehicle(registers& regs)
    {
        regs.ebx = renameVehicle(VehicleRenameArgs(regs), regs.bl);
    }

}
