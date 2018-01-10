#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <filesystem>
#include "../input.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"

namespace fs = std::experimental::filesystem;

namespace openloco::ui::windows
{
    loco_global_array<char, 256, 0x009D9D64> byte_9D9D64;
    loco_global_array<char, 32, 0x009D9E64> byte_9D9E64;
    loco_global_array<char, 512, 0x009D9E84> byte_9D9E84;
    loco_global_array<char, 512, 0x011369A0> byte_11369A0;

    void sub_446A93()
    {
        LOCO_CALLPROC_X(0x00446A93);
    }

    void sub_4CEB67(int16_t dx)
    {
        registers regs;
        regs.dx = dx;
        LOCO_CALLPROC_X(0x004CEB67, regs);
    }

    // 0x00445AB9
    // ecx: path
    // edx: filter
    // ebx: title
    // eax: {return}
    bool prompt_load_game(
        uint8_t al,
        char * path,
        const char * filter,
        const char * title)
    {
        textinput::close();

        LOCO_GLOBAL(0x009D9D63, uint8_t) = al;
        LOCO_GLOBAL(0x009DA284, uint8_t) = 0;
        if (filter == LOCO_ADDRESS(0x0050B512, char))
        {
            LOCO_GLOBAL(0x009DA284, uint8_t) = 1;
        }
        std::strcpy(byte_9D9D64, title);
        std::strcpy(byte_9D9E64, filter);
        std::strcpy(byte_9D9E84, path);

        auto baseName = fs::path(path).stem();
        if (baseName == ".")
        {
            baseName = "";
        }
        std::strcpy(byte_11369A0, baseName.generic_u8string().c_str());

        sub_446A93();
        auto window = windowmgr::create_window_centred(window_type::load_game, 500, 380, 0x1202, (void *)0x004FB308);
        if (window != nullptr)
        {
            window->widgets = (widget *)0x0050AD58;
            window->enabled_widgets = 4 | 0x10 | 0x40;
            window->sub_4CA17F();
            LOCO_GLOBAL(0x01136FA2, int16_t) = -1;
            LOCO_GLOBAL(0x011370A9, uint8_t) = 0;
            LOCO_GLOBAL(0x01136FA4, int16_t) = 0;
            window->var_83E = 11;
            window->var_85A = 0xFFFF;
            LOCO_GLOBAL(0x009DA285, uint8_t) = 0;
            sub_4CEB67(LOCO_GLOBAL(0x0050ADAC, int16_t) - LOCO_GLOBAL(0x0050ADAA, int16_t));
            window->var_886 = 0;
            window->var_887 = 11;
            LOCO_GLOBAL(0x005233B6, uint8_t) = 52;

            prompt_tick_loop(
                []()
                {
                    input::handle_keyboard();
                    sub_48A18C();
                    LOCO_CALLPROC_X(0x004CD3D0);
                    LOCO_CALLPROC_X(0x004BEC5B);
                    windowmgr::update();
                    LOCO_CALLPROC_X(0x004C98CF);
                    LOCO_CALLPROC_X(0x004CF63B);
                    return windowmgr::find(window_type::load_game) != nullptr;
                });

            LOCO_GLOBAL(0x005233B6, uint8_t) = 0xFF;
            std::strcpy(path, byte_9D9E84);
            if (path[0] != '\0')
            {
                return true;
            }
        }
        return false;
    }
}
