#pragma once

#include <cstdint>

namespace OpenLoco
{
    using Year = uint16_t;

    enum class MonthId : uint8_t
    {
        january,
        february,
        march,
        april,
        may,
        june,
        july,
        august,
        september,
        october,
        november,
        december
    };

    struct Date
    {
        int32_t day = 0;
        MonthId month;
        Year year = 0;

        // 0x0112C810 originally used as a return argument in calcDate
        int32_t dayOfOlympiad = 0;

        Date() = default;
        Date(Year y, MonthId m, int32_t d)
            : day(d)
            , month(m)
            , year(y)
        {
        }
    };

    bool isLeapYear(const int year);

    uint32_t getCurrentDay();
    void setCurrentDay(const uint32_t day);
    MonthId getCurrentMonth();
    Year getCurrentYear();
    void setCurrentYear(const Year year);

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

    uint8_t getMonthTotalDay(Year year, MonthId month);
}
