#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/objectmgr.h"
#include "../stationmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::station
{
    static loco_global<uint8_t[256], 0x001136BA4> byte_1136BA4;

    // 0x0048F210
    window* open(uint16_t id)
    {
        registers regs;
        regs.dx = id;
        call(0x0048F210, regs);

        return (window*)(uintptr_t)regs.esi;
    }

    // 0x0048EF02
    static void draw_rating_bar(window& w, gfx::drawpixelinfo_t& dpi, int16_t x, int16_t y, uint8_t amount, colour_t colour)
    {
        registers regs;
        regs.al = amount;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (loco_ptr)&w;
        regs.edi = (loco_ptr)&dpi;
        regs.ebp = colour;
        call(0x0048EF02, regs);
    }

    // 0x0048ED2F
    void tab_2_scroll_paint(window& w, gfx::drawpixelinfo_t& dpi)
    {
        gfx::clear_single(dpi, colour::get_shade(w.colours[1], 4));

        const auto station = stationmgr::get(w.number);
        int16_t y = 0;
        for (int i = 0; i < 32; i++)
        {
            auto& cargo = station->cargo_stats[i];
            if (!cargo.empty())
            {
                auto cargoObj = objectmgr::get<cargo_object>(i);
                gfx::draw_string_494BBF(dpi, 1, y, 98, 0, string_ids::wcolour2_stringid2, &cargoObj->name);

                auto rating = cargo.rating;
                auto colour = colour::moss_green;
                if (rating < 100)
                {
                    colour = colour::dark_olive_green;
                    if (rating < 50)
                    {
                        colour = colour::saturated_red;
                    }
                }
                uint8_t amount = (rating * 327) / 256;
                draw_rating_bar(w, dpi, 100, y, amount, colour);

                uint16_t percent = rating / 2;
                gfx::draw_string_494B3F(dpi, 201, y, 0, string_ids::station_cargo_rating_percent, &percent);
                y += 10;
            }
        }
    }
}
