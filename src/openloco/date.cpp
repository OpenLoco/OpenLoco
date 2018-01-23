#include "date.h"
#include "interop/interop.hpp"
#include <limits>
#include <utility>

using namespace openloco::interop;

namespace openloco
{
    static loco_global<uint32_t, 0x00525E2C> _current_day;
    static loco_global<uint16_t, 0x00525E30> _day_counter;
    static loco_global<int16_t, 0x00525E32> _current_year;
    static loco_global<int8_t, 0x00525E34> _current_month;
    static loco_global<int8_t, 0x00525E35> _current_day_of_month;
    static loco_global<int32_t, 0x0112C810> _current_day_in_olympiad;

    uint32_t current_day()
    {
        return _current_day;
    }

    date current_date()
    {
        return date(_current_year, (month_id)(*_current_month), _current_day_of_month);
    }

    void set_date(const date& date)
    {
        _current_day_of_month = date.day;
        _current_month = (int8_t)date.month;
        _current_year = date.year;
    }

    bool update_day_counter()
    {
        bool result = false;
        constexpr uint16_t increment = 682; // ~17s

        // Check if counter is going to wrap
        if (_day_counter + increment > std::numeric_limits<uint16_t>::max())
        {
            _current_day++;
            result = true;
        }

        _day_counter += increment;
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

        // clang-format off
        static constexpr int16_t month_table[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
            32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
            64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
            96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
            128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158,
            160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
            192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
            224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
            256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285,
            288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318,
            320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349,
            352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382
        };
        // clang-format on

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
        auto month = (month_id)(month_table[day] >> 5);
        auto dayOfMonth = month_table[day] & 31;

        auto result = date(year, month, dayOfMonth);
        result.day_of_olympiad = day;
        return result;
    }
}
