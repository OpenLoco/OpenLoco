#pragma once

namespace openloco::multiplayer
{
    enum flags
    {
        flag_0 = 0,
        flag_1 = 1,
        flag_2 = 2,
        flag_3 = 3,
        flag_4 = 4,
        flag_5 = 5,
        flag_6 = 6,
        flag_7 = 7,
        flag_8 = 8,
        flag_9 = 9,
        flag_10 = 10,
        flag_11 = 11,
        flag_12 = 12,
        flag_13 = 13,
        flag_14 = 14,
        flag_15 = 15,
    };

    bool has_flag(flags flag);
    bool set_flag(flags flag);
    bool reset_flag(flags flag);
}
