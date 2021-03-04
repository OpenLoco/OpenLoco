#include "Scenario.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Economy.h"
#include "Graphics/Gfx.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Map/MapGenerator.h"
#include "Map/TileManager.h"
#include "Objects/CargoObject.h"
#include "Ptr.h"
#include "S5/S5.h"
#include "StationManager.h"
#include "Things/ThingManager.h"
#include "Title.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Windows/Construction/Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;

namespace OpenLoco::Scenario
{
    static loco_global<cargo_object*, 0x0050D15C> _50D15C;

    static loco_global<uint32_t, 0x00525F5E> _scenario_ticks;
    static loco_global<uint8_t, 0x00525FB5> _525FB5;
    static loco_global<uint16_t, 0x0052622E> _52622E; // tick-related?

    static loco_global<uint8_t, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoAmount;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526241> objectiveTimeLimitUntilYear;

    // 0x0046115C
    static void sub_46115C()
    {
        call(0x0046115C);
    }

    // 0x004C4BC0
    static void sub_4C4BC0()
    {
        call(0x004C4BC0);
    }

    // 0x00496A18
    static void sub_496A18()
    {
        call(0x00496A18);
    }

    // 0x00475988
    static void sub_475988()
    {
        call(0x00475988);
    }

    // 0x004A8810
    static void sub_4A8810()
    {
        call(0x004A8810);
    }

    // TODO: Move to OrderManager::reset
    // 0x004702EC
    static void sub_4702EC()
    {
        call(0x004702EC);
    }

    // TODO: Move to Terraform::reset
    // 0x004BAEC4
    static void sub_4BAEC4()
    {
        call(0x004BAEC4);
    }

    // 0x0043C8FD
    static void sub_43C8FD()
    {
        call(0x0043C8FD);
    }

    // 0x0043C88C
    void reset()
    {
        WindowManager::closeConstructionWindows();

        CompanyManager::updatingCompanyId(0x0F);
        WindowManager::setCurrentRotation(0);

        CompanyManager::reset();
        StringManager::reset();
        ThingManager::reset();

        Ui::Windows::Construction::Construction::reset();
        sub_46115C();
        sub_4C4BC0();

        initialiseDate(1900);

        sub_496A18();
        sub_475988();
        TownManager::reset();
        IndustryManager::reset();
        StationManager::reset();

        sub_4A8810();
        sub_4702EC();
        sub_4BAEC4();
        sub_43C8FD();
        Title::sub_4284C8();
    }

    // 0x004748D4
    // ?Set default types for building. Like the initial track type and vehicle type and such.?
    void sub_4748D4()
    {
        call(0x004748D4);
    }

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

    // 0x0043C90C
    void generateLandscape()
    {
        auto& options = S5::getOptions();
        MapGenerator::generate(options);
        options.madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
    }

    // 0x0049685C
    void initialiseDate(uint16_t year)
    {
        // NB: this base value was already 1800 in Locomotion.
        uint32_t dayCount = 0;
        for (int y = 1800; y < year; y++)
        {
            dayCount += 365;
            if (isLeapYear(y))
                dayCount += 1;
        }

        setDate(date(year, month_id::january, 1));
        setCurrentDay(dayCount);
        setDayProgression(0);

        _scenario_ticks = 0;
        _52622E = 0;
        _525FB5 = 1;

        CompanyManager::determineAvailableVehicles();

        Economy::sub_46E2C0(getCurrentYear());
    }

    void registerHooks()
    {
        registerHook(
            0x0043C88C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                reset();
                regs = backup;
                return 0;
            });

        registerHook(
            0x0043C90C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                generateLandscape();
                regs = backup;
                return 0;
            });

        registerHook(
            0x0049685C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                initialiseDate(regs.ax);
                regs = backup;
                return 0;
            });
    }

    // 0x0044400C
    void start(const char* filename)
    {
        if (filename == nullptr)
            filename = reinterpret_cast<const char*>(-1);

        registers regs;
        regs.ebx = ToInt(filename);
        call(0x0044400C, regs);
    }

    // this will prepare _commonFormatArgs array before drawing the StringIds::challenge_value
    // after that for example it will draw this string: Achieve a performance index of 10.0% ("Engineer")
    // Note: no input and output parameters are in the original assembly, update is done in the memory
    // in this implementation we return FormatArguments so in the future it will be not depending on global variables
    // 0x004384E9
    void formatChallengeArguments(FormatArguments& args)
    {
        switch (objectiveType)
        {
            case Scenario::objective_type::company_value:
                args.push(StringIds::achieve_a_company_value_of);
                args.push(*objectiveCompanyValue);
                break;

            case Scenario::objective_type::vehicle_profit:
                args.push(StringIds::achieve_a_monthly_profit_from_vehicles_of);
                args.push(*objectiveMonthlyVehicleProfit);
                break;

            case Scenario::objective_type::performance_index:
            {
                args.push(StringIds::achieve_a_performance_index_of);
                int16_t performanceIndex = objectivePerformanceIndex * 10;
                formatPerformanceIndex(performanceIndex, args);
                break;
            }

            case Scenario::objective_type::cargo_delivery:
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

        if ((objectiveFlags & Scenario::objective_flags::be_top_company) != 0)
        {
            args.push(StringIds::and_be_the_top_performing_company);
        }
        if ((objectiveFlags & Scenario::objective_flags::be_within_top_three_companies) != 0)
        {
            args.push(StringIds::and_be_one_of_the_top_3_performing_companies);
        }
        if ((objectiveFlags & Scenario::objective_flags::within_time_limit) != 0)
        {
            if (isTitleMode() || isEditorMode())
            {
                args.push(StringIds::within_years);
                args.push<uint16_t>(*objectiveTimeLimitYears);
            }
            else
            {
                args.push(StringIds::by_the_end_of);
                args.push(*objectiveTimeLimitUntilYear);
            }
        }

        args.push<uint16_t>(0);
        args.push<uint16_t>(0);
    }
}
