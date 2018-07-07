#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/objectmgr.h"
#include "../stationmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::windows::StationWindow
{
    static station_id_t getStationId(const Window& w)
    {
        return w.number;
    }

    static station& getStation(const Window& w)
    {
        return *(stationmgr::get(getStationId(w)));
    }

    // 0x0048EF02
    static void drawRatingBar(Window& w, gfx::GraphicsContext& context, int16_t x, int16_t y, uint8_t amount, colour_t colour)
    {
        registers regs;
        regs.al = amount;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)&w;
        regs.edi = (int32_t)&context;
        regs.ebp = colour;
        call(0x0048EF02, regs);
    }

    // 0x0048ED2F
    void drawScroll2(Window& w, gfx::GraphicsContext& context)
    {
        auto paletteId = colour::get_shade(w.colours[1], 4);
        gfx::clear_single(context, paletteId);

        const auto& station = getStation(w);
        int16_t y = 0;
        for (int i = 0; i < 32; i++)
        {
            auto& cargo = station.cargo_stats[i];
            if (!cargo.empty())
            {
                auto cargoObj = objectmgr::get<cargo_object>(i);
                gfx::draw_string_494BBF(context, 1, y, 98, 0, string_ids::wcolour2_stringid2, &cargoObj->name);

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
                drawRatingBar(w, context, 100, y, amount, colour);

                uint16_t percent = rating / 2;
                gfx::draw_string_494B3F(context, 201, y, 0, string_ids::station_cargo_rating_percent, &percent);
                y += 10;
            }
        }
    }
}
