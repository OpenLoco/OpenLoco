#include "Date.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include <limits>
#include <utility>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    static loco_global<int32_t, 0x0112C810> _currentDayInOlympiad;

    static std::pair<MonthId, uint8_t> getMonthDay(int32_t dayOfYear);

    bool isLeapYear(const int year)
    {
        return year % 4 == 0;
    }

    uint32_t getCurrentDay()
    {
        return getGameState().currentDay;
    }

    void setCurrentDay(const uint32_t day)
    {
        getGameState().currentDay = day;
    }

    MonthId getCurrentMonth()
    {
        return MonthId(getGameState().currentMonth);
    }

    static void setCurrentMonth(const MonthId month)
    {
        getGameState().currentMonth = enumValue(month);
    }

    uint16_t getCurrentYear()
    {
        return getGameState().currentYear;
    }

    void setCurrentYear(const int16_t year)
    {
        getGameState().currentYear = year;
    }

    int8_t getCurrentDayOfMonth()
    {
        return getGameState().currentDayOfMonth;
    }

    static void setCurrentDayOfMonth(const int8_t day)
    {
        getGameState().currentDayOfMonth = day;
    }

    Date getCurrentDate()
    {
        return Date(getCurrentYear(), getCurrentMonth(), getCurrentDayOfMonth() + 1);
    }

    void setDate(const Date& date)
    {
        setCurrentDayOfMonth(date.day - 1);
        setCurrentMonth(date.month);
        setCurrentYear(date.year);
    }

    uint16_t getDayProgression()
    {
        return getGameState().dayCounter;
    }

    void setDayProgression(const uint16_t progression)
    {
        getGameState().dayCounter = progression;
    }

    bool updateDayCounter()
    {
        bool result = false;
        constexpr uint16_t kIncrement = 682; // ~17s

        // Check if counter is going to wrap
        if (getGameState().dayCounter + kIncrement > std::numeric_limits<uint16_t>::max())
        {
            getGameState().currentDay++;
            result = true;
        }

        getGameState().dayCounter += kIncrement;
        return result;
    }

    // 0x00495C65
    // eax: totalDays
    // returns:
    //    eax: year
    //    ebx: month
    //    edx: day
    Date calcDate(const uint32_t totalDays)
    {
        constexpr auto kBaseYear = 1800;
        constexpr auto kDaysInYear = 365;
        constexpr auto kDaysInOlympiad = (365 * 4) + 1;
        constexpr auto kFeb29 = 31 + 28;

        int32_t years = ((totalDays / kDaysInOlympiad) & 0xFFFF) * 4;
        int32_t day = totalDays % kDaysInOlympiad;

        // Count the years and add an extra day for when there isn't leap year
        if (day > kDaysInYear)
        {
            day -= kDaysInYear;
            day -= 1;
            do
            {
                years++;
                day -= kDaysInYear;
            } while (day >= 0);
            day += kDaysInYear;
            if (day >= kFeb29)
            {
                day++;
            }
        }

        _currentDayInOlympiad = day;

        const auto year = kBaseYear + years;
        const auto monthDay = getMonthDay(day);

        auto result = Date(year, monthDay.first, monthDay.second);
        result.dayOfOlympiad = day;
        return result;
    }

    uint32_t calcDays(Date date)
    {
        constexpr auto kBaseYear = 1800;
        constexpr auto kDaysInYear = 365;

        // adds years (365 for each year + 1 for leap years)
        auto yearDiff = date.year - kBaseYear;
        uint32_t dayCount = (yearDiff * kDaysInYear) + (yearDiff / 4);

        // add months
        for (int month = 0; month < static_cast<uint8_t>(date.month); ++month)
        {
            dayCount += getMonthTotalDay(date.year, static_cast<MonthId>(month));
        }

        // add days
        dayCount += date.day;

        return dayCount;
    }

    static std::pair<MonthId, uint8_t> getMonthDay(int32_t dayOfYear)
    {
        static constexpr std::pair<MonthId, uint8_t> month_table[] = {
            { MonthId::january, 1 },
            { MonthId::january, 2 },
            { MonthId::january, 3 },
            { MonthId::january, 4 },
            { MonthId::january, 5 },
            { MonthId::january, 6 },
            { MonthId::january, 7 },
            { MonthId::january, 8 },
            { MonthId::january, 9 },
            { MonthId::january, 10 },
            { MonthId::january, 11 },
            { MonthId::january, 12 },
            { MonthId::january, 13 },
            { MonthId::january, 14 },
            { MonthId::january, 15 },
            { MonthId::january, 16 },
            { MonthId::january, 17 },
            { MonthId::january, 18 },
            { MonthId::january, 19 },
            { MonthId::january, 20 },
            { MonthId::january, 21 },
            { MonthId::january, 22 },
            { MonthId::january, 23 },
            { MonthId::january, 24 },
            { MonthId::january, 25 },
            { MonthId::january, 26 },
            { MonthId::january, 27 },
            { MonthId::january, 28 },
            { MonthId::january, 29 },
            { MonthId::january, 30 },
            { MonthId::january, 31 },
            { MonthId::february, 1 },
            { MonthId::february, 2 },
            { MonthId::february, 3 },
            { MonthId::february, 4 },
            { MonthId::february, 5 },
            { MonthId::february, 6 },
            { MonthId::february, 7 },
            { MonthId::february, 8 },
            { MonthId::february, 9 },
            { MonthId::february, 10 },
            { MonthId::february, 11 },
            { MonthId::february, 12 },
            { MonthId::february, 13 },
            { MonthId::february, 14 },
            { MonthId::february, 15 },
            { MonthId::february, 16 },
            { MonthId::february, 17 },
            { MonthId::february, 18 },
            { MonthId::february, 19 },
            { MonthId::february, 20 },
            { MonthId::february, 21 },
            { MonthId::february, 22 },
            { MonthId::february, 23 },
            { MonthId::february, 24 },
            { MonthId::february, 25 },
            { MonthId::february, 26 },
            { MonthId::february, 27 },
            { MonthId::february, 28 },
            { MonthId::february, 29 },
            { MonthId::march, 1 },
            { MonthId::march, 2 },
            { MonthId::march, 3 },
            { MonthId::march, 4 },
            { MonthId::march, 5 },
            { MonthId::march, 6 },
            { MonthId::march, 7 },
            { MonthId::march, 8 },
            { MonthId::march, 9 },
            { MonthId::march, 10 },
            { MonthId::march, 11 },
            { MonthId::march, 12 },
            { MonthId::march, 13 },
            { MonthId::march, 14 },
            { MonthId::march, 15 },
            { MonthId::march, 16 },
            { MonthId::march, 17 },
            { MonthId::march, 18 },
            { MonthId::march, 19 },
            { MonthId::march, 20 },
            { MonthId::march, 21 },
            { MonthId::march, 22 },
            { MonthId::march, 23 },
            { MonthId::march, 24 },
            { MonthId::march, 25 },
            { MonthId::march, 26 },
            { MonthId::march, 27 },
            { MonthId::march, 28 },
            { MonthId::march, 29 },
            { MonthId::march, 30 },
            { MonthId::march, 31 },
            { MonthId::april, 1 },
            { MonthId::april, 2 },
            { MonthId::april, 3 },
            { MonthId::april, 4 },
            { MonthId::april, 5 },
            { MonthId::april, 6 },
            { MonthId::april, 7 },
            { MonthId::april, 8 },
            { MonthId::april, 9 },
            { MonthId::april, 10 },
            { MonthId::april, 11 },
            { MonthId::april, 12 },
            { MonthId::april, 13 },
            { MonthId::april, 14 },
            { MonthId::april, 15 },
            { MonthId::april, 16 },
            { MonthId::april, 17 },
            { MonthId::april, 18 },
            { MonthId::april, 19 },
            { MonthId::april, 20 },
            { MonthId::april, 21 },
            { MonthId::april, 22 },
            { MonthId::april, 23 },
            { MonthId::april, 24 },
            { MonthId::april, 25 },
            { MonthId::april, 26 },
            { MonthId::april, 27 },
            { MonthId::april, 28 },
            { MonthId::april, 29 },
            { MonthId::april, 30 },
            { MonthId::may, 1 },
            { MonthId::may, 2 },
            { MonthId::may, 3 },
            { MonthId::may, 4 },
            { MonthId::may, 5 },
            { MonthId::may, 6 },
            { MonthId::may, 7 },
            { MonthId::may, 8 },
            { MonthId::may, 9 },
            { MonthId::may, 10 },
            { MonthId::may, 11 },
            { MonthId::may, 12 },
            { MonthId::may, 13 },
            { MonthId::may, 14 },
            { MonthId::may, 15 },
            { MonthId::may, 16 },
            { MonthId::may, 17 },
            { MonthId::may, 18 },
            { MonthId::may, 19 },
            { MonthId::may, 20 },
            { MonthId::may, 21 },
            { MonthId::may, 22 },
            { MonthId::may, 23 },
            { MonthId::may, 24 },
            { MonthId::may, 25 },
            { MonthId::may, 26 },
            { MonthId::may, 27 },
            { MonthId::may, 28 },
            { MonthId::may, 29 },
            { MonthId::may, 30 },
            { MonthId::may, 31 },
            { MonthId::june, 1 },
            { MonthId::june, 2 },
            { MonthId::june, 3 },
            { MonthId::june, 4 },
            { MonthId::june, 5 },
            { MonthId::june, 6 },
            { MonthId::june, 7 },
            { MonthId::june, 8 },
            { MonthId::june, 9 },
            { MonthId::june, 10 },
            { MonthId::june, 11 },
            { MonthId::june, 12 },
            { MonthId::june, 13 },
            { MonthId::june, 14 },
            { MonthId::june, 15 },
            { MonthId::june, 16 },
            { MonthId::june, 17 },
            { MonthId::june, 18 },
            { MonthId::june, 19 },
            { MonthId::june, 20 },
            { MonthId::june, 21 },
            { MonthId::june, 22 },
            { MonthId::june, 23 },
            { MonthId::june, 24 },
            { MonthId::june, 25 },
            { MonthId::june, 26 },
            { MonthId::june, 27 },
            { MonthId::june, 28 },
            { MonthId::june, 29 },
            { MonthId::june, 30 },
            { MonthId::july, 1 },
            { MonthId::july, 2 },
            { MonthId::july, 3 },
            { MonthId::july, 4 },
            { MonthId::july, 5 },
            { MonthId::july, 6 },
            { MonthId::july, 7 },
            { MonthId::july, 8 },
            { MonthId::july, 9 },
            { MonthId::july, 10 },
            { MonthId::july, 11 },
            { MonthId::july, 12 },
            { MonthId::july, 13 },
            { MonthId::july, 14 },
            { MonthId::july, 15 },
            { MonthId::july, 16 },
            { MonthId::july, 17 },
            { MonthId::july, 18 },
            { MonthId::july, 19 },
            { MonthId::july, 20 },
            { MonthId::july, 21 },
            { MonthId::july, 22 },
            { MonthId::july, 23 },
            { MonthId::july, 24 },
            { MonthId::july, 25 },
            { MonthId::july, 26 },
            { MonthId::july, 27 },
            { MonthId::july, 28 },
            { MonthId::july, 29 },
            { MonthId::july, 30 },
            { MonthId::july, 31 },
            { MonthId::august, 1 },
            { MonthId::august, 2 },
            { MonthId::august, 3 },
            { MonthId::august, 4 },
            { MonthId::august, 5 },
            { MonthId::august, 6 },
            { MonthId::august, 7 },
            { MonthId::august, 8 },
            { MonthId::august, 9 },
            { MonthId::august, 10 },
            { MonthId::august, 11 },
            { MonthId::august, 12 },
            { MonthId::august, 13 },
            { MonthId::august, 14 },
            { MonthId::august, 15 },
            { MonthId::august, 16 },
            { MonthId::august, 17 },
            { MonthId::august, 18 },
            { MonthId::august, 19 },
            { MonthId::august, 20 },
            { MonthId::august, 21 },
            { MonthId::august, 22 },
            { MonthId::august, 23 },
            { MonthId::august, 24 },
            { MonthId::august, 25 },
            { MonthId::august, 26 },
            { MonthId::august, 27 },
            { MonthId::august, 28 },
            { MonthId::august, 29 },
            { MonthId::august, 30 },
            { MonthId::august, 31 },
            { MonthId::september, 1 },
            { MonthId::september, 2 },
            { MonthId::september, 3 },
            { MonthId::september, 4 },
            { MonthId::september, 5 },
            { MonthId::september, 6 },
            { MonthId::september, 7 },
            { MonthId::september, 8 },
            { MonthId::september, 9 },
            { MonthId::september, 10 },
            { MonthId::september, 11 },
            { MonthId::september, 12 },
            { MonthId::september, 13 },
            { MonthId::september, 14 },
            { MonthId::september, 15 },
            { MonthId::september, 16 },
            { MonthId::september, 17 },
            { MonthId::september, 18 },
            { MonthId::september, 19 },
            { MonthId::september, 20 },
            { MonthId::september, 21 },
            { MonthId::september, 22 },
            { MonthId::september, 23 },
            { MonthId::september, 24 },
            { MonthId::september, 25 },
            { MonthId::september, 26 },
            { MonthId::september, 27 },
            { MonthId::september, 28 },
            { MonthId::september, 29 },
            { MonthId::september, 30 },
            { MonthId::october, 1 },
            { MonthId::october, 2 },
            { MonthId::october, 3 },
            { MonthId::october, 4 },
            { MonthId::october, 5 },
            { MonthId::october, 6 },
            { MonthId::october, 7 },
            { MonthId::october, 8 },
            { MonthId::october, 9 },
            { MonthId::october, 10 },
            { MonthId::october, 11 },
            { MonthId::october, 12 },
            { MonthId::october, 13 },
            { MonthId::october, 14 },
            { MonthId::october, 15 },
            { MonthId::october, 16 },
            { MonthId::october, 17 },
            { MonthId::october, 18 },
            { MonthId::october, 19 },
            { MonthId::october, 20 },
            { MonthId::october, 21 },
            { MonthId::october, 22 },
            { MonthId::october, 23 },
            { MonthId::october, 24 },
            { MonthId::october, 25 },
            { MonthId::october, 26 },
            { MonthId::october, 27 },
            { MonthId::october, 28 },
            { MonthId::october, 29 },
            { MonthId::october, 30 },
            { MonthId::october, 31 },
            { MonthId::november, 1 },
            { MonthId::november, 2 },
            { MonthId::november, 3 },
            { MonthId::november, 4 },
            { MonthId::november, 5 },
            { MonthId::november, 6 },
            { MonthId::november, 7 },
            { MonthId::november, 8 },
            { MonthId::november, 9 },
            { MonthId::november, 10 },
            { MonthId::november, 11 },
            { MonthId::november, 12 },
            { MonthId::november, 13 },
            { MonthId::november, 14 },
            { MonthId::november, 15 },
            { MonthId::november, 16 },
            { MonthId::november, 17 },
            { MonthId::november, 18 },
            { MonthId::november, 19 },
            { MonthId::november, 20 },
            { MonthId::november, 21 },
            { MonthId::november, 22 },
            { MonthId::november, 23 },
            { MonthId::november, 24 },
            { MonthId::november, 25 },
            { MonthId::november, 26 },
            { MonthId::november, 27 },
            { MonthId::november, 28 },
            { MonthId::november, 29 },
            { MonthId::november, 30 },
            { MonthId::december, 1 },
            { MonthId::december, 2 },
            { MonthId::december, 3 },
            { MonthId::december, 4 },
            { MonthId::december, 5 },
            { MonthId::december, 6 },
            { MonthId::december, 7 },
            { MonthId::december, 8 },
            { MonthId::december, 9 },
            { MonthId::december, 10 },
            { MonthId::december, 11 },
            { MonthId::december, 12 },
            { MonthId::december, 13 },
            { MonthId::december, 14 },
            { MonthId::december, 15 },
            { MonthId::december, 16 },
            { MonthId::december, 17 },
            { MonthId::december, 18 },
            { MonthId::december, 19 },
            { MonthId::december, 20 },
            { MonthId::december, 21 },
            { MonthId::december, 22 },
            { MonthId::december, 23 },
            { MonthId::december, 24 },
            { MonthId::december, 25 },
            { MonthId::december, 26 },
            { MonthId::december, 27 },
            { MonthId::december, 28 },
            { MonthId::december, 29 },
            { MonthId::december, 30 },
            { MonthId::december, 31 },
        };
        return month_table[dayOfYear];
    }

    uint8_t getMonthTotalDay(uint16_t year, MonthId month)
    {
        static constexpr std::pair<MonthId, uint8_t> month_table[] = {
            { MonthId::january, 31 },
            { MonthId::february, 28 },
            { MonthId::march, 31 },
            { MonthId::april, 30 },
            { MonthId::may, 31 },
            { MonthId::june, 30 },
            { MonthId::july, 31 },
            { MonthId::august, 31 },
            { MonthId::september, 30 },
            { MonthId::october, 31 },
            { MonthId::november, 30 },
            { MonthId::december, 31 },
        };
        bool extraDay = month == MonthId::february && isLeapYear(year);
        return month_table[(uint8_t)month].second + extraDay;
    }
}
