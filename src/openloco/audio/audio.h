#pragma once

#include "../types.hpp"

namespace openloco::audio
{
    enum class sound_id
    {
        click_1 = 0,
        sound_1 = 1,
        sound_2 = 2,
        sound_14 = 14,
    };

    void initialise_dsound();
    void dispose_dsound();
    void initialise();

    void play_sound(sound_id id, loc16 loc);
    void play_sound(sound_id id, loc16 loc, int32_t pan);
    void play_sound(sound_id id, int32_t pan);
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency, bool obj_sound);
}
