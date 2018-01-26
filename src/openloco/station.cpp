#include "station.h"
#include "company.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include "windowmgr.h"
#include <algorithm>

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco
{
    station_id_t station::id()
    {
        // TODO check if this is stored in station structure
        //      otherwise add it when possible
        static loco_global<station[1024], 0x005E6EDC> _stations;
        auto index = (size_t)(this - _stations);
        if (index > 1024)
        {
            index = station_id::null;
        }
        return (station_id_t)index;
    }

    // 0x0048F7D1
    void station::sub_48F7D1()
    {
        registers regs;
        regs.ebx = id();
        call(0x0048F7D1, regs);
    }

    // 0x00492793
    bool station::update_cargo()
    {
        bool atLeastOneGoodRating = false;
        bool quantityUpdated = false;

        var_3B0 = std::min(var_3B0 + 1, 255);
        var_3B1 = std::min(var_3B1 + 1, 255);

        for (int i = 0; i < 32; i++)
        {
            auto& cargo = cargo_stats[i];
            if (!cargo.empty())
            {
                if (cargo.quantity != 0 && cargo.origin != id())
                {
                    cargo.enroute_age = std::min(cargo.enroute_age + 1, 255);
                }
                cargo.age = std::min(cargo.age + 1, 255);

                auto targetRating = calculate_cargo_rating(cargo);
                // Limit to +/- 2 minimum change
                auto ratingDelta = std::clamp(targetRating - cargo.rating, -2, 2);
                cargo.rating += ratingDelta;

                if (cargo.rating <= 50)
                {
                    // Rating < 25%, decrease cargo
                    if (cargo.quantity >= 400)
                    {
                        cargo.quantity -= rand_next(1, 32);
                        quantityUpdated = true;
                    }
                    else if (cargo.quantity >= 200)
                    {
                        cargo.quantity -= rand_next(1, 8);
                        quantityUpdated = true;
                    }
                }
                if (cargo.rating >= 100)
                {
                    atLeastOneGoodRating = true;
                }
                if (cargo.rating <= 100 && cargo.quantity != 0)
                {
                    if (cargo.rating <= rand_next(0, 127))
                    {
                        cargo.quantity = std::max(0, cargo.quantity - rand_next(1, 4));
                        quantityUpdated = true;
                    }
                }
            }
        }

        sub_4929DB();

        auto w = windowmgr::find(window_type::station, id());
        if (w != nullptr && (w->var_870 == 2 || w->var_870 == 1 || quantityUpdated))
        {
            w->invalidate();
        }

        return atLeastOneGoodRating;
    }

    int32_t station::calculate_cargo_rating(const station_cargo_stats& cargo) const
    {
        int32_t rating = 0;

        // Bonus if cargo is fresh
        if (cargo.age <= 45)
        {
            rating += 40;
            if (cargo.age <= 30)
            {
                rating += 45;
                if (cargo.age <= 15)
                {
                    rating += 45;
                    if (cargo.age <= 7)
                    {
                        rating += 35;
                    }
                }
            }
        }

        // Penalty if lots of cargo waiting
        rating -= 130;
        if (cargo.quantity <= 1000)
        {
            rating += 30;
            if (cargo.quantity <= 500)
            {
                rating += 30;
                if (cargo.quantity <= 300)
                {
                    rating += 30;
                    if (cargo.quantity <= 200)
                    {
                        rating += 20;
                        if (cargo.quantity <= 100)
                        {
                            rating += 20;
                        }
                    }
                }
            }
        }

        if (var_2A != 384 && is_player_company(company))
        {
            rating += 120;
        }

        int32_t unk3 = std::min<uint8_t>(cargo.var_36, 250);
        if (unk3 < 35)
        {
            rating += unk3 / 4;
        }

        if (cargo.var_38 < 4)
        {
            rating += 10;
            if (cargo.var_38 < 2)
            {
                rating += 10;
                if (cargo.var_38 < 1)
                {
                    rating += 13;
                }
            }
        }

        return std::clamp(0, rating, 200);
    }

    void station::sub_4929DB()
    {
        registers regs;
        regs.ebp = (int32_t)this;
        call(0x004929DB, regs);
    }

    // 0x004CBA2D
    void station::invalidate()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x004CBA2D, regs);
    }
}
