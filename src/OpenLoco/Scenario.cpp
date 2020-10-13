#include "Scenario.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Objects/CargoObject.h"
#include "S5/S5.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Scenario
{
    static loco_global<cargo_object*, 0x0050D15C> _50D15C;
    static loco_global<uint8_t, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoAmount;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526241> _526241; // objective complete goal until year?

    // 0x0043EDAD
    void eraseLandscape()
    {
        S5::getOptions().scenarioFlags &= ~(Scenario::flags::landscape_generation_done);
        Ui::WindowManager::invalidate(Ui::WindowType::landscapeGeneration, 0);
        call(0x0043C88C);
        S5::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        Gfx::invalidateScreen();
    }

    void generateLandscape()
    {
        call(0x0043C90C);
    }

    // 0x0044400C
    void start(const char* filename)
    {
        if (filename == nullptr)
            filename = reinterpret_cast<const char*>(-1);

        registers regs;
        regs.ebx = reinterpret_cast<int32_t>(filename);
        call(0x0044400C, regs);
    }

    // this will prepare _commonFormatArgs array before drawing the StringIds::challenge_value
    // after that for example it will draw this string: Achieve a performance index of 10.0% ("Engineer")
    // Note: no input and output parameters are in the original assembly, update is done in the memory
    // in this implementation we return FormatArguments so in the future it will be not depending on global variables
    // 0x004384E9
    void formatChallengeArguments(FormatArguments& args)
    {
        // 004384E
        switch (objectiveType)
        {
            case Scenario::objective_type::company_value: // 004384FB-0043850A
                args.push(StringIds::achieve_a_company_value_of);
                args.push(*objectiveCompanyValue);
                break;

            case Scenario::objective_type::vehicle_profit: // 0043850C-0043851B
                args.push(StringIds::achieve_a_monthly_profit_from_vehicles_of);
                args.push(*objectiveMonthlyVehicleProfit);
                break;

            case Scenario::objective_type::performance_index: // 0043851D-00438543
            {
                args.push(StringIds::achieve_a_performance_index_of);
                int16_t performanceIndex = objectivePerformanceIndex * 10;
                formatPerformanceIndex(performanceIndex, args);
                break;
            }

            case Scenario::objective_type::cargo_delivery: //00438545-00438576
            {
                args.push(StringIds::deliver);
                cargo_object* cargoObject = _50D15C;
                if (objectiveDeliveredCargoType != 0xFF)
                {
                    cargoObject = ObjectManager::get<cargo_object>(objectiveDeliveredCargoType);
                }
                args.push(cargoObject->unit_name_plural);
                args.push(*objectiveDeliveredCargoAmount);
                break;
            }
        }

        if ((objectiveFlags & Scenario::objective_flags::be_top_company) != 0) // 0043857B
        {
            args.push(StringIds::and_be_the_top_performing_company);
        }
        if ((objectiveFlags & Scenario::objective_flags::be_within_top_three_companies) != 0) // 0043858C
        {
            args.push(StringIds::and_be_one_of_the_top_3_performing_companies);
        }
        if ((objectiveFlags & Scenario::objective_flags::within_time_limit) != 0) // 0043859D
        {
            if (isTitleMode() || isEditorMode())
            {
                // 004385C5-004385D6
                args.push(StringIds::within_years);
                args.push<uint16_t>(*objectiveTimeLimitYears);
            }
            else
            {
                // 004385B1-004385C3
                args.push(StringIds::by_the_end_of);
                args.push(*_526241);
            }
        }

        // 004385D9
        args.push<uint16_t>(0);
        args.push<uint16_t>(0);
    }
}
