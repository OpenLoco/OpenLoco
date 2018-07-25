#include "audio.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::audio
{
    [[maybe_unused]] constexpr int32_t play_at_centre = 0x8000;
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
        call(0x004899E4);
    }

    // 0x00489C34
    void pause_sound()
    {
        call(0x00489C34);
    }

    // 0x00489CB5
    void play_sound(sound_id id, loc16 loc)
    {
        play_sound(id, loc, play_at_location);
    }

    // 0x00489CB5
    void play_sound(sound_id id, int32_t pan)
    {
        play_sound(id, {}, play_at_location);
    }

    // 0x00489F1B
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency, bool obj_sound)
    {
        registers regs;
        regs.eax = (int32_t)id;
        regs.eax |= obj_sound ? 0x8000 : 0;
        regs.ecx = loc.x;
        regs.edx = loc.y;
        regs.ebp = loc.z;
        regs.ebx = frequency;
        regs.edi = volume;
        call(0x00489F1B, regs);
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
