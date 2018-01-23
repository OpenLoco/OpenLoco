#pragma once

#include <cstdint>

namespace openloco
{
    enum class month_id
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

    struct date
    {
        int32_t day = 0;
        month_id month;
        int32_t year = 0;

        // 0x0112C810 originally used as a return argument in calc_date
        int32_t day_of_olympiad = 0;

        date() = default;
        date(int32_t y, month_id m, int32_t d)
            : year(y)
            , month(m)
            , day(d)
        {
        }
    };

    uint32_t current_day();

    date current_date();
    void set_date(const date& date);

    /**
     * Updates the current day counter.
     * @returns true if the counter wraps indicating a new day.
     */
    bool update_day_counter();
    date calc_date(uint32_t totalDays);
}
