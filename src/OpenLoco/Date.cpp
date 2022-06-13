#include "Date.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include <limits>
#include <utility>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    static loco_global<int32_t, 0x0112C810> _currentDayInOlympiad;

    static std::pair<Month, uint8_t> getMonthDay(int32_t dayOfYear);

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

    Month getCurrentMonth()
    {
        return Month(getGameState().currentMonth);
    }

    static void setCurrentMonth(const Month month)
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
            dayCount += getMonthTotalDay(date.year, static_cast<Month>(month));
        }

        // add days
        dayCount += date.day;

        return dayCount;
    }

    static std::pair<Month, uint8_t> getMonthDay(int32_t dayOfYear)
    {
        static constexpr std::pair<Month, uint8_t> month_table[] = {
            { Month::January, 1 },
            { Month::January, 2 },
            { Month::January, 3 },
            { Month::January, 4 },
            { Month::January, 5 },
            { Month::January, 6 },
            { Month::January, 7 },
            { Month::January, 8 },
            { Month::January, 9 },
            { Month::January, 10 },
            { Month::January, 11 },
            { Month::January, 12 },
            { Month::January, 13 },
            { Month::January, 14 },
            { Month::January, 15 },
            { Month::January, 16 },
            { Month::January, 17 },
            { Month::January, 18 },
            { Month::January, 19 },
            { Month::January, 20 },
            { Month::January, 21 },
            { Month::January, 22 },
            { Month::January, 23 },
            { Month::January, 24 },
            { Month::January, 25 },
            { Month::January, 26 },
            { Month::January, 27 },
            { Month::January, 28 },
            { Month::January, 29 },
            { Month::January, 30 },
            { Month::January, 31 },
            { Month::February, 1 },
            { Month::February, 2 },
            { Month::February, 3 },
            { Month::February, 4 },
            { Month::February, 5 },
            { Month::February, 6 },
            { Month::February, 7 },
            { Month::February, 8 },
            { Month::February, 9 },
            { Month::February, 10 },
            { Month::February, 11 },
            { Month::February, 12 },
            { Month::February, 13 },
            { Month::February, 14 },
            { Month::February, 15 },
            { Month::February, 16 },
            { Month::February, 17 },
            { Month::February, 18 },
            { Month::February, 19 },
            { Month::February, 20 },
            { Month::February, 21 },
            { Month::February, 22 },
            { Month::February, 23 },
            { Month::February, 24 },
            { Month::February, 25 },
            { Month::February, 26 },
            { Month::February, 27 },
            { Month::February, 28 },
            { Month::February, 29 },
            { Month::March, 1 },
            { Month::March, 2 },
            { Month::March, 3 },
            { Month::March, 4 },
            { Month::March, 5 },
            { Month::March, 6 },
            { Month::March, 7 },
            { Month::March, 8 },
            { Month::March, 9 },
            { Month::March, 10 },
            { Month::March, 11 },
            { Month::March, 12 },
            { Month::March, 13 },
            { Month::March, 14 },
            { Month::March, 15 },
            { Month::March, 16 },
            { Month::March, 17 },
            { Month::March, 18 },
            { Month::March, 19 },
            { Month::March, 20 },
            { Month::March, 21 },
            { Month::March, 22 },
            { Month::March, 23 },
            { Month::March, 24 },
            { Month::March, 25 },
            { Month::March, 26 },
            { Month::March, 27 },
            { Month::March, 28 },
            { Month::March, 29 },
            { Month::March, 30 },
            { Month::March, 31 },
            { Month::April, 1 },
            { Month::April, 2 },
            { Month::April, 3 },
            { Month::April, 4 },
            { Month::April, 5 },
            { Month::April, 6 },
            { Month::April, 7 },
            { Month::April, 8 },
            { Month::April, 9 },
            { Month::April, 10 },
            { Month::April, 11 },
            { Month::April, 12 },
            { Month::April, 13 },
            { Month::April, 14 },
            { Month::April, 15 },
            { Month::April, 16 },
            { Month::April, 17 },
            { Month::April, 18 },
            { Month::April, 19 },
            { Month::April, 20 },
            { Month::April, 21 },
            { Month::April, 22 },
            { Month::April, 23 },
            { Month::April, 24 },
            { Month::April, 25 },
            { Month::April, 26 },
            { Month::April, 27 },
            { Month::April, 28 },
            { Month::April, 29 },
            { Month::April, 30 },
            { Month::May, 1 },
            { Month::May, 2 },
            { Month::May, 3 },
            { Month::May, 4 },
            { Month::May, 5 },
            { Month::May, 6 },
            { Month::May, 7 },
            { Month::May, 8 },
            { Month::May, 9 },
            { Month::May, 10 },
            { Month::May, 11 },
            { Month::May, 12 },
            { Month::May, 13 },
            { Month::May, 14 },
            { Month::May, 15 },
            { Month::May, 16 },
            { Month::May, 17 },
            { Month::May, 18 },
            { Month::May, 19 },
            { Month::May, 20 },
            { Month::May, 21 },
            { Month::May, 22 },
            { Month::May, 23 },
            { Month::May, 24 },
            { Month::May, 25 },
            { Month::May, 26 },
            { Month::May, 27 },
            { Month::May, 28 },
            { Month::May, 29 },
            { Month::May, 30 },
            { Month::May, 31 },
            { Month::June, 1 },
            { Month::June, 2 },
            { Month::June, 3 },
            { Month::June, 4 },
            { Month::June, 5 },
            { Month::June, 6 },
            { Month::June, 7 },
            { Month::June, 8 },
            { Month::June, 9 },
            { Month::June, 10 },
            { Month::June, 11 },
            { Month::June, 12 },
            { Month::June, 13 },
            { Month::June, 14 },
            { Month::June, 15 },
            { Month::June, 16 },
            { Month::June, 17 },
            { Month::June, 18 },
            { Month::June, 19 },
            { Month::June, 20 },
            { Month::June, 21 },
            { Month::June, 22 },
            { Month::June, 23 },
            { Month::June, 24 },
            { Month::June, 25 },
            { Month::June, 26 },
            { Month::June, 27 },
            { Month::June, 28 },
            { Month::June, 29 },
            { Month::June, 30 },
            { Month::July, 1 },
            { Month::July, 2 },
            { Month::July, 3 },
            { Month::July, 4 },
            { Month::July, 5 },
            { Month::July, 6 },
            { Month::July, 7 },
            { Month::July, 8 },
            { Month::July, 9 },
            { Month::July, 10 },
            { Month::July, 11 },
            { Month::July, 12 },
            { Month::July, 13 },
            { Month::July, 14 },
            { Month::July, 15 },
            { Month::July, 16 },
            { Month::July, 17 },
            { Month::July, 18 },
            { Month::July, 19 },
            { Month::July, 20 },
            { Month::July, 21 },
            { Month::July, 22 },
            { Month::July, 23 },
            { Month::July, 24 },
            { Month::July, 25 },
            { Month::July, 26 },
            { Month::July, 27 },
            { Month::July, 28 },
            { Month::July, 29 },
            { Month::July, 30 },
            { Month::July, 31 },
            { Month::August, 1 },
            { Month::August, 2 },
            { Month::August, 3 },
            { Month::August, 4 },
            { Month::August, 5 },
            { Month::August, 6 },
            { Month::August, 7 },
            { Month::August, 8 },
            { Month::August, 9 },
            { Month::August, 10 },
            { Month::August, 11 },
            { Month::August, 12 },
            { Month::August, 13 },
            { Month::August, 14 },
            { Month::August, 15 },
            { Month::August, 16 },
            { Month::August, 17 },
            { Month::August, 18 },
            { Month::August, 19 },
            { Month::August, 20 },
            { Month::August, 21 },
            { Month::August, 22 },
            { Month::August, 23 },
            { Month::August, 24 },
            { Month::August, 25 },
            { Month::August, 26 },
            { Month::August, 27 },
            { Month::August, 28 },
            { Month::August, 29 },
            { Month::August, 30 },
            { Month::August, 31 },
            { Month::September, 1 },
            { Month::September, 2 },
            { Month::September, 3 },
            { Month::September, 4 },
            { Month::September, 5 },
            { Month::September, 6 },
            { Month::September, 7 },
            { Month::September, 8 },
            { Month::September, 9 },
            { Month::September, 10 },
            { Month::September, 11 },
            { Month::September, 12 },
            { Month::September, 13 },
            { Month::September, 14 },
            { Month::September, 15 },
            { Month::September, 16 },
            { Month::September, 17 },
            { Month::September, 18 },
            { Month::September, 19 },
            { Month::September, 20 },
            { Month::September, 21 },
            { Month::September, 22 },
            { Month::September, 23 },
            { Month::September, 24 },
            { Month::September, 25 },
            { Month::September, 26 },
            { Month::September, 27 },
            { Month::September, 28 },
            { Month::September, 29 },
            { Month::September, 30 },
            { Month::October, 1 },
            { Month::October, 2 },
            { Month::October, 3 },
            { Month::October, 4 },
            { Month::October, 5 },
            { Month::October, 6 },
            { Month::October, 7 },
            { Month::October, 8 },
            { Month::October, 9 },
            { Month::October, 10 },
            { Month::October, 11 },
            { Month::October, 12 },
            { Month::October, 13 },
            { Month::October, 14 },
            { Month::October, 15 },
            { Month::October, 16 },
            { Month::October, 17 },
            { Month::October, 18 },
            { Month::October, 19 },
            { Month::October, 20 },
            { Month::October, 21 },
            { Month::October, 22 },
            { Month::October, 23 },
            { Month::October, 24 },
            { Month::October, 25 },
            { Month::October, 26 },
            { Month::October, 27 },
            { Month::October, 28 },
            { Month::October, 29 },
            { Month::October, 30 },
            { Month::October, 31 },
            { Month::November, 1 },
            { Month::November, 2 },
            { Month::November, 3 },
            { Month::November, 4 },
            { Month::November, 5 },
            { Month::November, 6 },
            { Month::November, 7 },
            { Month::November, 8 },
            { Month::November, 9 },
            { Month::November, 10 },
            { Month::November, 11 },
            { Month::November, 12 },
            { Month::November, 13 },
            { Month::November, 14 },
            { Month::November, 15 },
            { Month::November, 16 },
            { Month::November, 17 },
            { Month::November, 18 },
            { Month::November, 19 },
            { Month::November, 20 },
            { Month::November, 21 },
            { Month::November, 22 },
            { Month::November, 23 },
            { Month::November, 24 },
            { Month::November, 25 },
            { Month::November, 26 },
            { Month::November, 27 },
            { Month::November, 28 },
            { Month::November, 29 },
            { Month::November, 30 },
            { Month::December, 1 },
            { Month::December, 2 },
            { Month::December, 3 },
            { Month::December, 4 },
            { Month::December, 5 },
            { Month::December, 6 },
            { Month::December, 7 },
            { Month::December, 8 },
            { Month::December, 9 },
            { Month::December, 10 },
            { Month::December, 11 },
            { Month::December, 12 },
            { Month::December, 13 },
            { Month::December, 14 },
            { Month::December, 15 },
            { Month::December, 16 },
            { Month::December, 17 },
            { Month::December, 18 },
            { Month::December, 19 },
            { Month::December, 20 },
            { Month::December, 21 },
            { Month::December, 22 },
            { Month::December, 23 },
            { Month::December, 24 },
            { Month::December, 25 },
            { Month::December, 26 },
            { Month::December, 27 },
            { Month::December, 28 },
            { Month::December, 29 },
            { Month::December, 30 },
            { Month::December, 31 },
        };
        return month_table[dayOfYear];
    }

    uint8_t getMonthTotalDay(uint16_t year, Month month)
    {
        static constexpr std::pair<Month, uint8_t> month_table[] = {
            { Month::January, 31 },
            { Month::February, 28 },
            { Month::March, 31 },
            { Month::April, 30 },
            { Month::May, 31 },
            { Month::June, 30 },
            { Month::July, 31 },
            { Month::August, 31 },
            { Month::September, 30 },
            { Month::October, 31 },
            { Month::November, 30 },
            { Month::December, 31 },
        };
        bool extraDay = month == Month::February && isLeapYear(year);
        return month_table[(uint8_t)month].second + extraDay;
    }
}
