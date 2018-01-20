#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <setjmp.h>

// timeGetTime is unavailable if we use lean and mean
// #define WIN32_LEAN_AND_MEAN
#define NOMINMAX
//#include <windows.h>
//#include <objbase.h>

#include "audio/audio.h"
#include "config.h"
#include "environment.h"
#include "graphics/gfx.h"
#include "input.h"
#include "interop/interop.hpp"
#include "intro.h"
#include "objects/objectmgr.h"
#include "openloco.h"
#include "progressbar.h"
#include "scenariomgr.h"
#include "things/thingmgr.h"
#include "tutorial.h"
#include "ui.h"
#include "utility/numeric.hpp"
#include "windowmgr.h"

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

using namespace openloco::interop;
namespace windowmgr = openloco::ui::windowmgr;
using input_flags = openloco::input::input_flags;
using input_state = openloco::input::input_state;
using window_type = openloco::ui::window_type;

namespace openloco
{

long timeGetTime() {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    return spec.tv_nsec / 1000000;
}

    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";

//    loco_global<HINSTANCE, 0x0113E0B4> ghInstance;
//    loco_global<LPSTR, 0x00525348> glpCmdLine;
    loco_global_array<char, 256, 0x005060D0> gCDKey;

    loco_global<uint16_t, 0x0050C19C> time_since_last_tick;
    loco_global<uint32_t, 0x0050C19E> last_tick_time;
    loco_global<uint8_t, 0x00508F08> game_command_nest_level;
    loco_global<uint16_t, 0x00508F12> _screen_age;
    loco_global<uint8_t, 0x00508F14> _screen_flags;
    loco_global<uint8_t, 0x00508F17> paused_state;
    loco_global<uint8_t, 0x00508F1A> game_speed;
    loco_global<uint8_t, 0x0050AF26> byte_50AF26;
    loco_global<uint32_t, 0x00525E18> _srand0;
    loco_global<uint32_t, 0x00525E1C> _srand1;
    loco_global<uint32_t, 0x00525F5E> _scenario_ticks;

    void tick_logic(int32_t count);
    void tick_logic();
    void tick_wait();

    void * hInstance()
    {
        return nullptr;
    }

    const char * lpCmdLine()
    {
        return nullptr;
    }

    bool is_editor_mode()
    {
        return (_screen_flags & screen_flags::editor) != 0;
    }

    bool is_title_mode()
    {
        return (_screen_flags & screen_flags::title) != 0;
    }

    bool is_paused()
    {
        return paused_state;
    }

    uint32_t scenario_ticks()
    {
        return _scenario_ticks;
    }

    bool sub_4054B9()
    {
        registers regs;
        call(0x004054B9, regs);
        return regs.eax != 0;
    }

    /**
     * Use this to allocate memory that will be freed in vanilla code or via loco_free.
     */
    void * malloc(size_t size)
    {
        return ((void *(*)(size_t))0x004D1401)(size);
    }

    /**
     * Use this to reallocate memory that will be freed in vanilla code or via loco_free.
     */
    void * realloc(void * address, size_t size)
    {
        return ((void *(*)(void *, size_t))0x004D1B28)(address, size);
    }

    /**
     * Use this to free up memory allocated in vanilla code or via loco_malloc / loco_realloc.
     */
    void free(void * address)
    {
        ((void(*)(void *))0x004D1355)(address);
    }

    void sub_404E58()
    {
        free(addr<0x005251F4, void *>());
        addr<0x005251F4, void *>() = nullptr;
        addr<0x005251F0, void *>() = nullptr;
        call(0x00404ACD);
        call(0x00404B40);
    }

    void sub_4062D1()
    {
        call(0x004062D1);
    }

    void sub_406417()
    {
        //((void(*)())0x00406417)();
    }

    void sub_40567E()
    {
        call(0x0040567E);
    }

    void sub_4058F5()
    {
        call(0x004058F5);
    }

    void sub_4062E0()
    {
        call(0x004062E0);
    }

    // eax: width
    // ebx: height
    bool sub_451F0B(int32_t width, int32_t height)
    {
        registers regs;
        regs.eax = width;
        regs.ebx = height;
        call(0x00451F0B, regs);
        return regs.al != 0;
    }

    void sub_4BE621(int32_t eax, int32_t ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        call(0x004BE621, regs);
    }

    void sub_45235D()
    {
        call(0x00452336);
        int32_t width = addr<0x0050AEB8, int16_t>();
        int32_t height = addr<0x0050AEBA, int16_t>();
        if (addr<0x0050AEC0, uint8_t>() != 0xFF || width == -1)
        {
            // int32_t screenWidth = addr<0x00113E2C8, int32_t>();
            int32_t screenHeight = addr<0x00113E2CC, int32_t>();
            width = 1024;
            height = 768;
            if (screenHeight < 1200)
            {
                width = 800;
                height = 600;
            }
        }
        if (sub_451F0B(width, height))
        {
            addr<0x0052533C, int32_t>() = 0;
            if (addr<0x0052532C, int32_t>() == 0 &&
                addr<0x00113E2E4, int32_t>() >= 64)
            {
                call(0x004524C1);
                call(0x004523F4);
            }
            else
            {
                sub_4BE621(61, 62);
            }
        }
        else
        {
            sub_4BE621(61, 63);
        }
    }

    bool sub_4034FC(int32_t &a, int32_t &b)
    {
        auto result = ((int32_t(*)(int32_t &, int32_t &))(0x004034FC))(a, b);
        return result != 0;
    }

    void sub_431A8A(uint16_t bx, uint16_t dx)
    {
        registers regs;
        regs.bx = bx;
        regs.dx = dx;
        call(0x00431A8A, regs);
    }

    // 0x00407FFD
    bool is_already_running(const char * mutexName)
    {
        return false;
        auto result = ((int32_t(*)(const char *))(0x00407FFD))(mutexName);
        return result != 0;
    }

    // 0x004BE621
    void exit_with_error(string_id eax, string_id ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        call(0x004BE621, regs);
    }

    // 0x0044155B
    void check_game_file_exists(int i)
    {
        registers regs;
        regs.ebx = i;
        call(0x0044155B, regs);
    }

    // 0x0044154B
    void check_game_files_exist()
    {
        for (int i = 0; i < 48; i++)
        {
            check_game_file_exists(i);
        }
    }

    // 0x004414C5
    void check_game_files_are_valid()
    {
        call(0x004414C5);
    }

    void sub_441444()
    {
        //call(0x00441444);
    }

    // 0x00441400
    void startup_checks()
    {
        if (is_already_running("Locomotion"))
        {
            exit_with_error(61, 1016);
        }
        else
        {
            check_game_files_exist();
            check_game_files_are_valid();
            sub_441444();
        }
    }

    // 0x004C57C0
    void initialise_viewports()
    {
        call(0x004C57C0);
    }

    void initialise()
    {
        addr<0x0050C18C, int32_t>() = addr<0x00525348, int32_t>();
        call(0x004078BE);
        //call(0x004BF476);
        environment::resolve_paths();
        progressbar::begin(1088, 0);
        progressbar::increment(30);
        startup_checks();
        progressbar::increment(40);
        call(0x004BE5DE);
        progressbar::end();
        config::read();
        //objectmgr::load_index();
        scenariomgr::load_index(0);
        progressbar::begin(1088, 0);
        progressbar::increment(60);
        gfx::load_g1();
        progressbar::increment(220);
        call(0x004949BC);
        progressbar::increment(235);
        progressbar::increment(250);
        ui::initialise_cursors();
        progressbar::end();
        ui::initialise();
        audio::initialise();
        initialise_viewports();
        call(0x004284C8);
        call(0x004969DA);
        call(0x0043C88C);
        addr<0x00508F14, int16_t>() |= 0x20;
//#ifdef _SHOW_INTRO_
        intro::state(intro::intro_state::begin);
//#else
//        intro::state(intro::intro_state::end);
//#endif
        call(0x0046AD7D);
        call(0x00438A6C);
        gfx::clear(gfx::screen_dpi(), 0x0A0A0A0A);
    }

    // 0x0046A794
    void tick()
    {
        static bool isInitialised = false;

        // Locomotion has several routines that will prematurely end the current tick.
        // This usually happens when switching game mode. It does this by jumping to
        // the end of the original routine and resetting esp back to an initial value
        // stored at the beginning of tick. Until those routines are re-written, we
        // must simulate it using 'setjmp'.
        static jmp_buf tickJump;

        // When Locomotion wants to jump to the end of a tick, it sets ESP
        // to some static memory that we define
        static loco_global<void *, 0x0050C1A6> tickJumpESP;
        static uint8_t spareStackMemory[2048];
        tickJumpESP = spareStackMemory + sizeof(spareStackMemory);

        if (setjmp(tickJump))
        {
            // Premature end of current tick
            std::cout << "tick prematurely ended" << std::endl;
            return;
        }

        addr<0x00113E87C, int32_t>() = 0;
        addr<0x0005252E0, int32_t>() = 0;
        if (!isInitialised)
        {
            isInitialised = true;

            // This address is where those routines jump back to to end the tick prematurely
            register_hook(0x0046AD71,
                [](registers &regs) -> uint8_t
                {
                    longjmp(tickJump, 1);
                });

            initialise();
            last_tick_time = timeGetTime();
        }

        uint32_t time = timeGetTime();
        time_since_last_tick = std::min(time - last_tick_time, 500U);
        last_tick_time = time;

        if (!is_paused())
        {
            addr<0x0050C1A2, uint32_t>() += time_since_last_tick;
        }
        if (tutorial::state() != tutorial::tutorial_state::none)
        {
            time_since_last_tick = 31;
        }
        game_command_nest_level = 0;
        ui::update();

        addr<0x005233AE, int32_t>() += addr<0x0114084C, int32_t>();
        addr<0x005233B2, int32_t>() += addr<0x01140840, int32_t>();
        addr<0x0114084C, int32_t>() = 0;
        addr<0x01140840, int32_t>() = 0;
        if (byte_50AF26 == 0)
        {
            byte_50AF26 = 16;
            gfx::clear(gfx::screen_dpi(), 0);
            ui::get_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
            addr<0x00F2539C, int32_t>() = 0;
        }
        else
        {
            if (byte_50AF26 >= 16)
            {
                byte_50AF26++;
                if (byte_50AF26 >= 48)
                {
                    if (sub_4034FC(addr<0x00F25394, int32_t>(), addr<0x00F25398, int32_t>()))
                    {
                        uintptr_t esi = addr<0x00F25390, int32_t>() + 4;
                        esi *= addr<0x00F25398, int32_t>();
                        esi += addr<0x00F2538C, int32_t>();
                        esi += 2;
                        esi += addr<0x00F25394, int32_t>();
                        addr<0x00F2539C, int32_t>() |= *((int32_t *)esi);
                        call(0x00403575);
                    }
                }
                ui::set_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                gfx::invalidate_screen();
                if (byte_50AF26 != 96)
                {
                    tick_wait();
                    return;
                }
                byte_50AF26 = 1;
                if (addr<0x00F2539C, int32_t>() != 0)
                {
                    byte_50AF26 = 2;
                }
                config::write();
            }

            call(0x00452D1A);
            //call(0x00440DEC);

            if (addr<0x00525340, int32_t>() == 1)
            {
                addr<0x00525340, int32_t>() = 0;
                addr<0x00508F10, uint16_t>() |= (1 << 1);
            }

            input::handle_keyboard();
            sub_48A18C();

            addr<0x0050C1AE, int32_t>()++;
            if (intro::is_active())
            {
                intro::update();
            }
            else
            {
                uint16_t numUpdates = std::clamp<uint16_t>(time_since_last_tick / (uint16_t)31, 1, 3);
                if (windowmgr::find(window_type::window_39, 0) != nullptr)
                {
                    numUpdates = 1;
                }
                if (_screen_flags & screen_flags::unknown_2)
                {
                    numUpdates = 1;
                }
                if (addr<0x00525324, int32_t>() == 1)
                {
                    addr<0x00525324, int32_t>() = 0;
                    numUpdates = 1;
                }
                else
                {
                    switch (input::state())
                    {
                        case input_state::reset:
                        case input_state::normal:
                        case input_state::dropdown_active:
                            if (input::has_flag(input_flags::viewport_scrolling))
                            {
                                input::reset_flag(input_flags::viewport_scrolling);
                                numUpdates = 1;
                            }
                            break;
                        case input_state::widget_pressed:break;
                        case input_state::positioning_window:break;
                        case input_state::viewport_right:break;
                        case input_state::viewport_left:break;
                        case input_state::scroll_left:break;
                        case input_state::resizing:break;
                        case input_state::scroll_right:break;
                    }
                }
                addr<0x0052622E, int16_t>() += numUpdates;
                if (is_paused())
                {
                    numUpdates = 0;
                }
                addr<0x00F253A0, uint16_t>() = std::max<uint16_t>(1, numUpdates);
                _screen_age = std::min(0xFFFF, (int32_t)_screen_age + addr<0x00F253A0, int16_t>());
                if (game_speed != 0)
                {
                    numUpdates *= 3;
                    if (game_speed != 1)
                    {
                        numUpdates *= 3;
                    }
                }

                call(0x0046FFCA);
                tick_logic(numUpdates);

                addr<0x00525F62, int16_t>()++;
                call(0x0043D9D4);
                call(0x0048A78D);
                call(0x0048AC66);
                if (tutorial::state() != tutorial::tutorial_state::none &&
                    addr<0x0052532C, int32_t>() == 0 &&
                    addr<0x0113E2E4, int32_t>() < 0x40)
                {
                    tutorial::stop();

                    // This ends with a premature tick termination
                    call(0x0043C0FD);
                    return; // won't be reached
                }

                call(0x00431695);
                call(0x00452B5F);
                call(0x0046FFCA);
                if (addr<0x0050AEC0, uint8_t>() != 0xFF)
                {
                    addr<0x0050AEC0, uint8_t>()++;
                    if (addr<0x0050AEC0, uint8_t>() != 0xFF)
                    {
                        config::write();
                    }
                }
            }

            if (byte_50AF26 == 2)
            {
                addr<0x005252DC, int32_t>() = 1;
                ui::get_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                ui::set_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
            }
        }

        tick_wait();
    }

    void tick_logic(int32_t count)
    {
        for (int32_t i = 0; i < count; i++)
        {
            tick_logic();
        }
    }

    // 0x004612EC
    void invalidate_map_animations()
    {
        call(0x004612EC);
    }

    // 0x0046ABCB
    void tick_logic()
    {
        _scenario_ticks++;
        addr<0x00525F64, int32_t>()++;
        addr<0x00525FCC, int32_t>() = _srand0;
        addr<0x00525FD0, int32_t>() = _srand1;
        call(0x004613F0);
        addr<0x00F25374, uint8_t>() = addr<0x009C871C, uint8_t>();
        call(0x004968C7);
        call(0x00463ABA);
        call(0x004C56F6);
        call(0x00496B6D);
        call(0x00453234);
        thingmgr::update_vehicles();
        call(0x0046FFCA);
        call(0x0048B1FA);
        thingmgr::update_misc_things();
        call(0x0046FFCA);
        call(0x00430319);
        invalidate_map_animations();
        call(0x0048A73B);
        call(0x0048ACFD);
        call(0x00444387);

        addr<0x009C871C, uint8_t>() = addr<0x00F25374, uint8_t>();
        if (addr<0x0050C197, uint8_t>() != 0)
        {
            uint16_t bx = 0x043A;
            uint16_t dx = addr<0x0050C198, uint16_t>();
            if (addr<0x0050C197, uint8_t>() == 0xFE)
            {
                bx = addr<0x0050C198, uint16_t>();
                dx = 0xFFFF;
            }
            addr<0x0050C197, uint8_t>() = 0;
            sub_431A8A(bx, dx);
        }
    }

    // 0x0046AD4D
    void tick_wait()
    {
        do
        {
            // Idle loop for a 40 FPS
        }
        while (timeGetTime() - last_tick_time < 25);
    }

    void prompt_tick_loop(std::function<bool()> tickAction)
    {
        while (true)
        {
            auto startTime = timeGetTime();
            time_since_last_tick = 31;
            if (!ui::process_messages() || !tickAction())
            {
                break;
            }
            ui::render();
            do
            {
                // Idle loop for a 40 FPS
            }
            while (timeGetTime() - startTime < 25);
        }
    }

    void sub_48A18C()
    {
        call(0x0048A18C);
    }

    uint32_t rand_next()
    {
        auto srand0 = _srand0;
        auto srand1 = _srand1;
        _srand0 = utility::ror<uint32_t>(srand1 ^ 0x1234567F, 7);
        _srand1 = utility::ror<uint32_t>(srand0, 3);
        return _srand1;
    }

    int32_t rand_next(int32_t high)
    {
        return rand_next(0, high);
    }

    int32_t rand_next(int32_t low, int32_t high)
    {
        int32_t positive = rand_next() & 0x7FFFFFFF;
        return low + (positive % ((high + 1) - low));
    }

    // 0x00406386
    void run()
    {
//        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        sub_4062D1();
        sub_406417();

#ifdef _READ_REGISTRY_
        constexpr auto INSTALL_REG_KEY = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{77F45E76-E897-42CA-A9FE-5F56817D875C}";

        HKEY key;
        if (RegOpenKeyA(HKEY_LOCAL_MACHINE, INSTALL_REG_KEY, &key) == ERROR_SUCCESS)
        {
            DWORD type;
            DWORD dataSize = gCDKey.size();
            RegQueryValueExA(key, "CDKey", nullptr, &type, (LPBYTE)gCDKey.get(), &dataSize);
            RegCloseKey(key);
        }
#endif

        while (ui::process_messages())
        {
            if (addr<0x005252AC, uint32_t>() != 0)
            {
                sub_4058F5();
            }
            sub_4062E0();
            tick();
            ui::render();
        }
        sub_40567E();
//        CoUninitialize();
    }

/**
 * Loads RCT2's data model and remaps the addresses.
 * @returns true if the data integrity check succeeded, otherwise false.
 */
bool openrct2_setup_rct2_segment()
{
    // OpenRCT2 on Linux and macOS is wired to have the original Windows PE sections loaded
    // necessary. Windows does not need to do this as OpenRCT2 runs as a DLL loaded from the Windows PE.
    int len = 0x01429000 - 0x8a4000; // 0xB85000, 12079104 bytes or around 11.5MB
    int err = 0;
#if defined(USE_MMAP) && (defined(__unix__) || defined(__MACOSX__))
    #define RDATA_OFFSET 0x004A4000
	#define DATASEG_OFFSET 0x005E2000

	// Using PE-bear I was able to figure out all the needed addresses to be filled.
	// There are three sections to be loaded: .rdata, .data and .text, plus another
	// one to be mapped: DATASEG.
	// Out of the three, two can simply be mmapped into memory, while the third one,
	// .data has a virtual size which is much completely different to its file size
	// (even when taking page-alignment into consideration)
	//
	// The sections are as follows (dump from gdb)
	// [0]     0x401000->0x6f7000 at 0x00001000: .text ALLOC LOAD READONLY CODE HAS_CONTENTS
	// [1]     0x6f7000->0x8a325d at 0x002f7000: CODESEG ALLOC LOAD READONLY CODE HAS_CONTENTS
	// [2]     0x8a4000->0x9a5894 at 0x004a4000: .rdata ALLOC LOAD DATA HAS_CONTENTS
	// [3]     0x9a6000->0x9e2000 at 0x005a6000: .data ALLOC LOAD DATA HAS_CONTENTS
	// [4]     0x1428000->0x14282bc at 0x005e2000: DATASEG ALLOC LOAD DATA HAS_CONTENTS
	// [5]     0x1429000->0x1452000 at 0x005e3000: .cms_t ALLOC LOAD READONLY CODE HAS_CONTENTS
	// [6]     0x1452000->0x14aaf3e at 0x0060c000: .cms_d ALLOC LOAD DATA HAS_CONTENTS
	// [7]     0x14ab000->0x14ac58a at 0x00665000: .idata ALLOC LOAD READONLY DATA HAS_CONTENTS
	// [8]     0x14ad000->0x14b512f at 0x00667000: .rsrc ALLOC LOAD DATA HAS_CONTENTS
	//
	// .data section, however, has virtual size of 0xA81C3C, and so
	// 0x9a6000 + 0xA81C3C = 0x1427C3C, which after alignment to page size becomes
	// 0x1428000, which can be seen as next section, DATASEG
	//
	// The data is now loaded into memory with a linker script, which proves to
	// be more reliable, as mallocs that happen before we reach segment setup
	// could have already taken the space we need.

	// TODO: UGLY, UGLY HACK!
	//off_t file_size = 6750208;

	utf8 segmentDataPath[MAX_PATH];
	openrct2_get_segment_data_path(segmentDataPath, sizeof(segmentDataPath));
	fdData = open(segmentDataPath, O_RDONLY);
	if (fdData < 0)
	{
		log_fatal("failed to load openrct2_data");
		exit(1);
	}
	log_warning("%p", GOOD_PLACE_FOR_DATA_SEGMENT);
	segments = mmap((void *)(GOOD_PLACE_FOR_DATA_SEGMENT), len, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE, fdData, 0);
	log_warning("%p", segments);
	if ((uintptr_t)segments != GOOD_PLACE_FOR_DATA_SEGMENT) {
		perror("mmap");
		return false;
	}
#endif // defined(USE_MMAP) && (defined(__unix__) || defined(__MACOSX__))

#if defined(__unix__)
    int pageSize = getpagesize();
	int numPages = (len + pageSize - 1) / pageSize;
	unsigned char *dummy = malloc(numPages);

	err = mincore((void *)segments, len, dummy);
	bool pagesMissing = false;
	if (err != 0)
	{
		err = errno;
#ifdef __LINUX__
		// On Linux ENOMEM means all requested range is unmapped
		if (err != ENOMEM)
		{
			pagesMissing = true;
			perror("mincore");
		}
#else
		pagesMissing = true;
		perror("mincore");
#endif // __LINUX__
	} else {
		for (int i = 0; i < numPages; i++)
		{
			if (dummy[i] != 1)
			{
				pagesMissing = true;
				void *start = (void *)segments + i * pageSize;
				void *end = (void *)segments + (i + 1) * pageSize - 1;
				log_warning("required page %p - %p is not in memory!", start, end);
			}
		}
	}
	free(dummy);
	if (pagesMissing)
	{
		log_error("At least one of required pages was not found in memory. This can cause segfaults later on.");
	}
#if !defined(USE_MMAP)
	// section: text
	err = mprotect((void *)0x401000, 0x8a4000 - 0x401000, PROT_READ | PROT_EXEC | PROT_WRITE);
	if (err != 0)
	{
		perror("mprotect");
	}
#endif // !defined(USE_MMAP)
	// section: rw data
	err = mprotect((void *)segments, 0x01429000 - 0x8a4000, PROT_READ | PROT_WRITE);
	if (err != 0)
	{
		perror("mprotect");
	}
#endif // defined(__unix__)

#if defined(USE_MMAP) && defined(__WINDOWS__)
    segments = VirtualAlloc((void *)(GOOD_PLACE_FOR_DATA_SEGMENT), len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if ((uintptr_t)segments != GOOD_PLACE_FOR_DATA_SEGMENT) {
		log_error("VirtualAlloc, segments = %p, GetLastError = 0x%x", segments, GetLastError());
		return false;
	}

	utf8 segmentDataPath[MAX_PATH];
	openrct2_get_segment_data_path(segmentDataPath, sizeof(segmentDataPath));
	SDL_RWops * rw = SDL_RWFromFile(segmentDataPath, "rb");
	if (rw == NULL)
	{
		log_error("failed to load file");
		return false;
	}
	if (SDL_RWread(rw, segments, len, 1) != 1) {
		log_error("Unable to read chunk header!");
		return false;
	}
	SDL_RWclose(rw);
#endif // defined(USE_MMAP) && defined(__WINDOWS__)

    // Check that the expected data is at various addresses.
    // Start at 0x9a6000, which is start of .data, to skip the region containing addresses to DLL
    // calls, which can be changed by windows/wine loader.
//    const uint32_t c1 = sawyercoding_calculate_checksum((const uint8*)(segments + (uintptr_t)(0x009A6000 - 0x8a4000)), 0x009E0000 - 0x009A6000);
//    const uint32_t c2 = sawyercoding_calculate_checksum((const uint8*)(segments + (uintptr_t)(0x01428000 - 0x8a4000)), 0x014282BC - 0x01428000);
//    const uint32_t exp_c1 = 10114815;
//    const uint32_t exp_c2 = 23564;
//    if (c1 != exp_c1 || c2 != exp_c2) {
//        printf("c1 = %u, expected %u, match %d", c1, exp_c1, c1 == exp_c1);
//        printf("c2 = %u, expected %u, match %d", c2, exp_c2, c2 == exp_c2);
//        return false;
//    }

    return true;
}

    // 0x00406D13
    void main()
    {
        std::cout << "OpenLoco v0.1" << std::endl;
        try
        {
            register_hooks();
            if (sub_4054B9())
            {
                ui::create_window();
                call(0x004078FE);
                call(0x00407B26);
                ui::initialise_input();
             //   audio::initialise_dsound();
                run();
            //    audio::dispose_dsound();
                ui::dispose_cursors();
                ui::dispose_input();

                // TODO extra clean up code
            }
        }
        catch (const std::exception &ex)
        {
            std::cerr << ex.what() << std::endl;
        }
    }
}

extern "C"
{
//    /**
//     * The function that is called directly from the host application (loco.exe)'s WinMain. This will be removed when OpenLoco can
//     * be built as a stand alone application.
//     */
//    __declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
//    {
//        openloco::glpCmdLine = lpCmdLine;
//        openloco::ghInstance = hInstance;
//        openloco::main();
//        return 0;
//    }

int main() {
    openloco::main();
        return 0;
}
}
