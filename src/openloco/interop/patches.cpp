#include "../windowmgr.h"
#include "interop.hpp"
#include <cassert>
#include <limits>
#include <type_traits>

using namespace openloco;

// TODO: find a less monstrously awful way to set these things
namespace openloco::ui::windowmgr
{
    extern ui::window* _windows;
    extern ui::window* _windows_end;
}

static constexpr int old_window_limit = 12;
static constexpr int new_window_limit = 224;

static constexpr int old_viewport_limit = 10;
static constexpr int new_viewport_limit = 224;

// keep these well under 256, just in case something is byte-indexed or whatever
static_assert(new_window_limit   < 250);
static_assert(new_viewport_limit < 250);

// more viewports than windows --> crashes will happen
static_assert(new_viewport_limit >= new_window_limit - 2);

// TODO: free memory later...?
// TODO: make the new window limit configurable-at-startup-time (ensure that it's equal to the viewport limit!)
static void apply_patch_window_limit_increase()
{
    constexpr auto oldaddr_window_list        = 0x011370AC;
    constexpr auto oldaddr_new_window_pointer = 0x0113D754;

    static_assert((oldaddr_new_window_pointer - oldaddr_window_list) == (old_window_limit * sizeof(ui::window)));

    // locations in original Loco.exe subroutines that reference the addr of window_list
    static constexpr uint32_t refs_to_window_list[] = {
        0x00438425,
        0x00452BCB,
        0x0045F199,
        0x0046961A,
        0x00489DD6,
        0x0048A03F,
        0x0048A320,
        0x004B92B3,
        0x004B93A7,
        0x004BED21,
        0x004BF0A1,
        0x004C5821,
        0x004C5E56,
        0x004C6173,
        0x004C61BB,
        0x004C61D6,
        0x004C62BE,
        0x004C97F3,
        0x004C98C1,
        0x004C98DD,
        0x004C9976,
        0x004C99A1,
        0x004C9AA3,
        0x004C9B08,
        0x004C9B57,
        0x004C9C14,
        0x004C9D65,
        0x004C9E7E,
        0x004C9EFD,
        0x004C9F81,
        0x004C9F9B,
        0x004CA0F6,
        0x004CB974,
        0x004CB9A2,
        0x004CBA0A,
        0x004CC694,
        0x004CC6DC,
        0x004CC770,
        0x004CD297,
        0x004CD3EE,
        0x004CE439,
        0x004CED87,
        0x004CEE23,
        0x004CF46F,
    };
    static_assert(std::extent_v<decltype(refs_to_window_list)> == 44);

    // locations in original Loco.exe subroutines that reference the addr of new_window_pointer
    static constexpr uint32_t refs_to_new_window_pointer[] = {
        0x0043842B,
        0x00452BD1,
        0x00452BF7,
        0x0045F18D,
        0x0046960E,
        0x00489DCA,
        0x0048A033,
        0x0048A314,
        0x004B3171,
        0x004B3CE9,
        0x004B5610,
        0x004B92A7,
        0x004B93AD,
        0x004BED15,
        0x004BF095,
        0x004C581D,
        0x004C5E5C,
        0x004C5EB3,
        0x004C6103,
        0x004C6179,
        0x004C61AF,
        0x004C61CA,
        0x004C62B0,
        0x004C6A43,
        0x004C6B0B,
        0x004C97E7,
        0x004C98B5,
        0x004C98D1,
        0x004C996A,
        0x004C9995,
        0x004C9A97,
        0x004C9AFC,
        0x004C9B64,
        0x004C9B87,
        0x004C9C1A,
        0x004C9D6B,
        0x004C9E84,
        0x004C9F03,
        0x004C9F5F,
        0x004C9F65,
        0x004C9FBF,
        0x004C9FC9,
        0x004CA0E8,
        0x004CB97A,
        0x004CB9A8,
        0x004CBA10,
        0x004CC6A1,
        0x004CC6C8,
        0x004CC72A,
        0x004CC734,
        0x004CC75D,
        0x004CD2A1,
        0x004CD3E2,
        0x004CE43F,
        0x004CED8D,
        0x004CEE29,
        0x004CF463,
    };
    static_assert(std::extent_v<decltype(refs_to_new_window_pointer)> == 57);

    // allocate a new, larger buffer on the heap for window_list and new_window_pointer
    const void* buf = ::malloc((new_window_limit * sizeof(ui::window)) + sizeof(uint32_t));
    assert(buf != nullptr);

    // ensure that the buffer was allocated in a 32-bit-addressable location
    assert((uintptr_t)buf < std::numeric_limits<uint32_t>::max());

    uint32_t newaddr_window_list = (uint32_t)buf;
    uint32_t newaddr_new_window_pointer = newaddr_window_list + (new_window_limit * sizeof(ui::window));

    // make the window list empty to start out
    *(uint32_t*)newaddr_new_window_pointer = newaddr_window_list;

    for (auto ref : refs_to_window_list)
    {
        ref = interop::remap_address(ref);

        uint32_t old;
        interop::read_memory(ref, &old, sizeof(old));
        assert(old == oldaddr_window_list);

        interop::write_memory(ref, &newaddr_window_list, sizeof(newaddr_window_list));
    }

    for (auto ref : refs_to_new_window_pointer)
    {
        ref = interop::remap_address(ref);

        uint32_t old;
        interop::read_memory(ref, &old, sizeof(old));
        assert(old == oldaddr_new_window_pointer);

        interop::write_memory(ref, &newaddr_new_window_pointer, sizeof(newaddr_new_window_pointer));
    }

    // HACK: make sure that our own windowmgr code uses the new pointers, not the old ones
    ui::windowmgr::_windows     = (ui::window*)newaddr_window_list;
    ui::windowmgr::_windows_end = (ui::window*)newaddr_new_window_pointer;
}


// TODO: free memory later...?
// TODO: make the new ciewport limit configurable-at-startup-time (ensure that it's equal to the window limit!)
static void apply_patch_viewport_limit_increase()
{
    constexpr auto oldaddr_viewport_list        = 0x0113D758;
    constexpr auto oldaddr_new_viewport_pointer = 0x0113D820;

    static_assert((oldaddr_new_viewport_pointer - oldaddr_viewport_list) == (old_viewport_limit * sizeof(ui::viewport)));

    // locations in original Loco.exe subroutines that reference the addr of viewport_list
    static constexpr uint32_t refs_to_viewport_list[] = {
        0x004C5830,
        0x004CA2D1,
        0x004CA38B,
        0x004CEC2D,
    };
    static_assert(std::extent_v<decltype(refs_to_viewport_list)> == 4);

    // locations in original Loco.exe subroutines that reference the addr of new_viewport_pointer
    static constexpr uint32_t refs_to_new_viewport_pointer[] = {
        0x004C5845,
        0x004CA2E0,
        0x004CA39A,
        0x004CBA31,
        0x004CBB1D,
        0x004CBBEE,
        0x004CBCC8,
        0x004CBDA2,
        0x004CBF19,
        0x004CC169,
        0x004CC2E0,
        0x004CC461,
        0x004CC5E2,
        0x004CEC28,
        0x004CEC41,
    };
    static_assert(std::extent_v<decltype(refs_to_new_viewport_pointer)> == 15);

    // allocate a new, larger buffer on the heap for viewport_list and new_viewport_pointer
    const void* buf = ::malloc((new_viewport_limit * sizeof(ui::viewport)) + sizeof(uint32_t));
    assert(buf != nullptr);

    // ensure that the buffer was allocated in a 32-bit-addressable location
    assert((uintptr_t)buf < std::numeric_limits<uint32_t>::max());

    uint32_t newaddr_viewport_list = (uint32_t)buf;
    uint32_t newaddr_new_viewport_pointer = newaddr_viewport_list + (new_viewport_limit * sizeof(ui::viewport));

    // make the viewport list empty to start out
    *(uint32_t*)newaddr_new_viewport_pointer = newaddr_viewport_list;

    for (auto ref : refs_to_viewport_list)
    {
        ref = interop::remap_address(ref);

        uint32_t old;
        interop::read_memory(ref, &old, sizeof(old));
        assert(old == oldaddr_viewport_list);

        interop::write_memory(ref, &newaddr_viewport_list, sizeof(newaddr_viewport_list));
    }

    for (auto ref : refs_to_new_viewport_pointer)
    {
        ref = interop::remap_address(ref);

        uint32_t old;
        interop::read_memory(ref, &old, sizeof(old));
        assert(old == oldaddr_new_viewport_pointer);

        interop::write_memory(ref, &newaddr_new_viewport_pointer, sizeof(newaddr_new_viewport_pointer));
    }

    // TODO: when viewport code is implemented, ensure that it uses our new pointers on the heap,
    // not the old ones in Loco.exe's .data section
}

void openloco::interop::apply_patches()
{
    apply_patch_window_limit_increase();
    apply_patch_viewport_limit_increase();
}
