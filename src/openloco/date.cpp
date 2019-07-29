#include "date.h"
#include "interop/interop.hpp"
#include <limits>
#include <utility>

using namespace openloco::interop;

namespace openloco
{
    static loco_global<uint32_t, 0x00525E2C> _current_day;
    static loco_global<uint16_t, 0x00525E30> _day_progression;
    static loco_global<int16_t, 0x00525E32> _current_year;
    static loco_global<int8_t, 0x00525E34> _current_month;
    static loco_global<int8_t, 0x00525E35> _current_day_of_month;
    static loco_global<int32_t, 0x0112C810> _current_day_in_olympiad;

    static std::pair<month_id, uint8_t> get_month_day(int32_t dayOfYear);

    uint32_t current_day()
    {
        return _current_day;
    }

    month_id current_month()
    {
        return static_cast<month_id>(*_current_month);
    }

    uint16_t current_year()
    {
        return _current_year;
    }

    void set_current_year(const int16_t year)
    {
        _current_year = year;
    }

    date current_date()
    {
        return date(_current_year, (month_id)(*_current_month), _current_day_of_month + 1);
    }

    void set_date(const date& date)
    {
        _current_day_of_month = date.day - 1;
        _current_month = (int8_t)date.month;
        _current_year = date.year;
    }

    bool update_day_counter()
    {
        bool result = false;
        constexpr uint16_t increment = 682; // ~17s

        // Check if counter is going to wrap
        if (_day_progression + increment > std::numeric_limits<uint16_t>::max())
        {
            _current_day++;
            result = true;
        }

        _day_progression += increment;
        return result;
    }

    // 0x00495C65
    // eax: totalDays
    // returns:
    //    eax: year
    //    ebx: month
    //    edx: day
    date calc_date(uint32_t totalDays)
    {
        constexpr auto base_year = 1800;
        constexpr auto days_in_year = 365;
        constexpr auto days_in_olympiad = (365 * 4) + 1;
        constexpr auto feb_29 = 31 + 28;

        int32_t years = ((totalDays / days_in_olympiad) & 0xFFFF) * 4;
        int32_t day = totalDays % days_in_olympiad;

        // Count the years and add an extra day for when there isn't leap year
        if (day > days_in_year)
        {
            day -= days_in_year;
            day -= 1;
            do
            {
                years++;
                day -= days_in_year;
            } while (day >= 0);
            day += days_in_year;
            if (day >= feb_29)
            {
                day++;
            }
        }

        _current_day_in_olympiad = day;

        auto year = base_year + years;
        auto monthDay = get_month_day(day);

        auto result = date(year, monthDay.first, monthDay.second);
        result.day_of_olympiad = day;
        return result;
    }

    static std::pair<month_id, uint8_t> get_month_day(int32_t dayOfYear)
    {
        // clang-format off
        static constexpr std::pair<month_id, uint8_t> month_table[] = {
            { month_id::january, 1 },
            { month_id::january, 2 },
            { month_id::january, 3 },
            { month_id::january, 4 },
            { month_id::january, 5 },
            { month_id::january, 6 },
            { month_id::january, 7 },
            { month_id::january, 8 },
            { month_id::january, 9 },
            { month_id::january, 10 },
            { month_id::january, 11 },
            { month_id::january, 12 },
            { month_id::january, 13 },
            { month_id::january, 14 },
            { month_id::january, 15 },
            { month_id::january, 16 },
            { month_id::january, 17 },
            { month_id::january, 18 },
            { month_id::january, 19 },
            { month_id::january, 20 },
            { month_id::january, 21 },
            { month_id::january, 22 },
            { month_id::january, 23 },
            { month_id::january, 24 },
            { month_id::january, 25 },
            { month_id::january, 26 },
            { month_id::january, 27 },
            { month_id::january, 28 },
            { month_id::january, 29 },
            { month_id::january, 30 },
            { month_id::january, 31 },
            { month_id::february, 1 },
            { month_id::february, 2 },
            { month_id::february, 3 },
            { month_id::february, 4 },
            { month_id::february, 5 },
            { month_id::february, 6 },
            { month_id::february, 7 },
            { month_id::february, 8 },
            { month_id::february, 9 },
            { month_id::february, 10 },
            { month_id::february, 11 },
            { month_id::february, 12 },
            { month_id::february, 13 },
            { month_id::february, 14 },
            { month_id::february, 15 },
            { month_id::february, 16 },
            { month_id::february, 17 },
            { month_id::february, 18 },
            { month_id::february, 19 },
            { month_id::february, 20 },
            { month_id::february, 21 },
            { month_id::february, 22 },
            { month_id::february, 23 },
            { month_id::february, 24 },
            { month_id::february, 25 },
            { month_id::february, 26 },
            { month_id::february, 27 },
            { month_id::february, 28 },
            { month_id::february, 29 },
            { month_id::march, 1 },
            { month_id::march, 2 },
            { month_id::march, 3 },
            { month_id::march, 4 },
            { month_id::march, 5 },
            { month_id::march, 6 },
            { month_id::march, 7 },
            { month_id::march, 8 },
            { month_id::march, 9 },
            { month_id::march, 10 },
            { month_id::march, 11 },
            { month_id::march, 12 },
            { month_id::march, 13 },
            { month_id::march, 14 },
            { month_id::march, 15 },
            { month_id::march, 16 },
            { month_id::march, 17 },
            { month_id::march, 18 },
            { month_id::march, 19 },
            { month_id::march, 20 },
            { month_id::march, 21 },
            { month_id::march, 22 },
            { month_id::march, 23 },
            { month_id::march, 24 },
            { month_id::march, 25 },
            { month_id::march, 26 },
            { month_id::march, 27 },
            { month_id::march, 28 },
            { month_id::march, 29 },
            { month_id::march, 30 },
            { month_id::march, 31 },
            { month_id::april, 1 },
            { month_id::april, 2 },
            { month_id::april, 3 },
            { month_id::april, 4 },
            { month_id::april, 5 },
            { month_id::april, 6 },
            { month_id::april, 7 },
            { month_id::april, 8 },
            { month_id::april, 9 },
            { month_id::april, 10 },
            { month_id::april, 11 },
            { month_id::april, 12 },
            { month_id::april, 13 },
            { month_id::april, 14 },
            { month_id::april, 15 },
            { month_id::april, 16 },
            { month_id::april, 17 },
            { month_id::april, 18 },
            { month_id::april, 19 },
            { month_id::april, 20 },
            { month_id::april, 21 },
            { month_id::april, 22 },
            { month_id::april, 23 },
            { month_id::april, 24 },
            { month_id::april, 25 },
            { month_id::april, 26 },
            { month_id::april, 27 },
            { month_id::april, 28 },
            { month_id::april, 29 },
            { month_id::april, 30 },
            { month_id::may, 1 },
            { month_id::may, 2 },
            { month_id::may, 3 },
            { month_id::may, 4 },
            { month_id::may, 5 },
            { month_id::may, 6 },
            { month_id::may, 7 },
            { month_id::may, 8 },
            { month_id::may, 9 },
            { month_id::may, 10 },
            { month_id::may, 11 },
            { month_id::may, 12 },
            { month_id::may, 13 },
            { month_id::may, 14 },
            { month_id::may, 15 },
            { month_id::may, 16 },
            { month_id::may, 17 },
            { month_id::may, 18 },
            { month_id::may, 19 },
            { month_id::may, 20 },
            { month_id::may, 21 },
            { month_id::may, 22 },
            { month_id::may, 23 },
            { month_id::may, 24 },
            { month_id::may, 25 },
            { month_id::may, 26 },
            { month_id::may, 27 },
            { month_id::may, 28 },
            { month_id::may, 29 },
            { month_id::may, 30 },
            { month_id::may, 31 },
            { month_id::june, 1 },
            { month_id::june, 2 },
            { month_id::june, 3 },
            { month_id::june, 4 },
            { month_id::june, 5 },
            { month_id::june, 6 },
            { month_id::june, 7 },
            { month_id::june, 8 },
            { month_id::june, 9 },
            { month_id::june, 10 },
            { month_id::june, 11 },
            { month_id::june, 12 },
            { month_id::june, 13 },
            { month_id::june, 14 },
            { month_id::june, 15 },
            { month_id::june, 16 },
            { month_id::june, 17 },
            { month_id::june, 18 },
            { month_id::june, 19 },
            { month_id::june, 20 },
            { month_id::june, 21 },
            { month_id::june, 22 },
            { month_id::june, 23 },
            { month_id::june, 24 },
            { month_id::june, 25 },
            { month_id::june, 26 },
            { month_id::june, 27 },
            { month_id::june, 28 },
            { month_id::june, 29 },
            { month_id::june, 30 },
            { month_id::july, 1 },
            { month_id::july, 2 },
            { month_id::july, 3 },
            { month_id::july, 4 },
            { month_id::july, 5 },
            { month_id::july, 6 },
            { month_id::july, 7 },
            { month_id::july, 8 },
            { month_id::july, 9 },
            { month_id::july, 10 },
            { month_id::july, 11 },
            { month_id::july, 12 },
            { month_id::july, 13 },
            { month_id::july, 14 },
            { month_id::july, 15 },
            { month_id::july, 16 },
            { month_id::july, 17 },
            { month_id::july, 18 },
            { month_id::july, 19 },
            { month_id::july, 20 },
            { month_id::july, 21 },
            { month_id::july, 22 },
            { month_id::july, 23 },
            { month_id::july, 24 },
            { month_id::july, 25 },
            { month_id::july, 26 },
            { month_id::july, 27 },
            { month_id::july, 28 },
            { month_id::july, 29 },
            { month_id::july, 30 },
            { month_id::july, 31 },
            { month_id::august, 1 },
            { month_id::august, 2 },
            { month_id::august, 3 },
            { month_id::august, 4 },
            { month_id::august, 5 },
            { month_id::august, 6 },
            { month_id::august, 7 },
            { month_id::august, 8 },
            { month_id::august, 9 },
            { month_id::august, 10 },
            { month_id::august, 11 },
            { month_id::august, 12 },
            { month_id::august, 13 },
            { month_id::august, 14 },
            { month_id::august, 15 },
            { month_id::august, 16 },
            { month_id::august, 17 },
            { month_id::august, 18 },
            { month_id::august, 19 },
            { month_id::august, 20 },
            { month_id::august, 21 },
            { month_id::august, 22 },
            { month_id::august, 23 },
            { month_id::august, 24 },
            { month_id::august, 25 },
            { month_id::august, 26 },
            { month_id::august, 27 },
            { month_id::august, 28 },
            { month_id::august, 29 },
            { month_id::august, 30 },
            { month_id::august, 31 },
            { month_id::september, 1 },
            { month_id::september, 2 },
            { month_id::september, 3 },
            { month_id::september, 4 },
            { month_id::september, 5 },
            { month_id::september, 6 },
            { month_id::september, 7 },
            { month_id::september, 8 },
            { month_id::september, 9 },
            { month_id::september, 10 },
            { month_id::september, 11 },
            { month_id::september, 12 },
            { month_id::september, 13 },
            { month_id::september, 14 },
            { month_id::september, 15 },
            { month_id::september, 16 },
            { month_id::september, 17 },
            { month_id::september, 18 },
            { month_id::september, 19 },
            { month_id::september, 20 },
            { month_id::september, 21 },
            { month_id::september, 22 },
            { month_id::september, 23 },
            { month_id::september, 24 },
            { month_id::september, 25 },
            { month_id::september, 26 },
            { month_id::september, 27 },
            { month_id::september, 28 },
            { month_id::september, 29 },
            { month_id::september, 30 },
            { month_id::october, 1 },
            { month_id::october, 2 },
            { month_id::october, 3 },
            { month_id::october, 4 },
            { month_id::october, 5 },
            { month_id::october, 6 },
            { month_id::october, 7 },
            { month_id::october, 8 },
            { month_id::october, 9 },
            { month_id::october, 10 },
            { month_id::october, 11 },
            { month_id::october, 12 },
            { month_id::october, 13 },
            { month_id::october, 14 },
            { month_id::october, 15 },
            { month_id::october, 16 },
            { month_id::october, 17 },
            { month_id::october, 18 },
            { month_id::october, 19 },
            { month_id::october, 20 },
            { month_id::october, 21 },
            { month_id::october, 22 },
            { month_id::october, 23 },
            { month_id::october, 24 },
            { month_id::october, 25 },
            { month_id::october, 26 },
            { month_id::october, 27 },
            { month_id::october, 28 },
            { month_id::october, 29 },
            { month_id::october, 30 },
            { month_id::october, 31 },
            { month_id::november, 1 },
            { month_id::november, 2 },
            { month_id::november, 3 },
            { month_id::november, 4 },
            { month_id::november, 5 },
            { month_id::november, 6 },
            { month_id::november, 7 },
            { month_id::november, 8 },
            { month_id::november, 9 },
            { month_id::november, 10 },
            { month_id::november, 11 },
            { month_id::november, 12 },
            { month_id::november, 13 },
            { month_id::november, 14 },
            { month_id::november, 15 },
            { month_id::november, 16 },
            { month_id::november, 17 },
            { month_id::november, 18 },
            { month_id::november, 19 },
            { month_id::november, 20 },
            { month_id::november, 21 },
            { month_id::november, 22 },
            { month_id::november, 23 },
            { month_id::november, 24 },
            { month_id::november, 25 },
            { month_id::november, 26 },
            { month_id::november, 27 },
            { month_id::november, 28 },
            { month_id::november, 29 },
            { month_id::november, 30 },
            { month_id::december, 1 },
            { month_id::december, 2 },
            { month_id::december, 3 },
            { month_id::december, 4 },
            { month_id::december, 5 },
            { month_id::december, 6 },
            { month_id::december, 7 },
            { month_id::december, 8 },
            { month_id::december, 9 },
            { month_id::december, 10 },
            { month_id::december, 11 },
            { month_id::december, 12 },
            { month_id::december, 13 },
            { month_id::december, 14 },
            { month_id::december, 15 },
            { month_id::december, 16 },
            { month_id::december, 17 },
            { month_id::december, 18 },
            { month_id::december, 19 },
            { month_id::december, 20 },
            { month_id::december, 21 },
            { month_id::december, 22 },
            { month_id::december, 23 },
            { month_id::december, 24 },
            { month_id::december, 25 },
            { month_id::december, 26 },
            { month_id::december, 27 },
            { month_id::december, 28 },
            { month_id::december, 29 },
            { month_id::december, 30 },
            { month_id::december, 31 },
        };
        // clang-format on
        return month_table[dayOfYear];
    }
}
