#pragma once

#include "../types.hpp"

namespace openloco::audio
{
    enum class sound_id
    {
    };

    void initialise_dsound();
    void dispose_dsound();
    void initialise();

    void play_sound(sound_id id, loc16 loc);
    void play_sound(sound_id id, loc16 loc, int32_t pan);
    void play_sound(sound_id id, int32_t pan);
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency, bool obj_sound);
}
