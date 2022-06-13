#pragma once

#include <cstdint>

namespace OpenLoco
{
    enum class Month : uint8_t
    {
        January,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December
    };

    struct Date
    {
        int32_t day = 0;
        Month month;
        int32_t year = 0;

        // 0x0112C810 originally used as a return argument in calcDate
        int32_t dayOfOlympiad = 0;

        Date() = default;
        Date(int32_t y, Month m, int32_t d)
            : day(d)
            , month(m)
            , year(y)
        {
        }
    };

    bool isLeapYear(const int year);

    uint32_t getCurrentDay();
    void setCurrentDay(const uint32_t day);
    Month getCurrentMonth();
    uint16_t getCurrentYear();
    void setCurrentYear(const int16_t year);

    Date getCurrentDate();
    void setDate(const Date& date);

    uint16_t getDayProgression();
    void setDayProgression(const uint16_t progression);

    int8_t getCurrentDayOfMonth();

    /**
     * Updates the current day counter.
     * @returns true if the counter wraps indicating a new day.
     */
    bool updateDayCounter();
    Date calcDate(uint32_t totalDays);
    uint32_t calcDays(Date date);

    uint8_t getMonthTotalDay(uint16_t year, Month month);
}
