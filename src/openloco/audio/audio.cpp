#include "../interop/interop.hpp"
#include "audio.h"

using namespace openloco::interop;

namespace openloco::audio
{
    constexpr int32_t play_at_centre = 0x8000;
    constexpr int32_t play_at_location = 0x8001;

    // 0x00404E53
    void initialise_dsound()
    {
        call(0x00404E53);
    }

    // 0x00404E58
    void dispose_dsound()
    {
        call(0x00404E58);
    }

    // 0x004899E4
    void initialise()
    {
       // call(0x004899E4);
    }

    // 0x00489CB5
    void play_sound(sound_id id, loc16 loc)
    {
        play_sound(id, loc, play_at_location);
    }

    void play_sound(sound_id id, loc16 loc, int32_t pan)
    {
        registers regs;
        regs.eax = (int32_t)id;
        regs.cx = loc.x;
        regs.dx = loc.y;
        regs.bp = loc.z;
        regs.ebx = pan;
        call(0x00489CB5, regs);
    }
}
