#include "townmgr.h"
#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include "ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::townmgr
{
    static loco_global<town[80], 0x005B825C> _towns;

    std::array<town, max_towns>& towns()
    {
        auto arr = (std::array<town, max_towns>*)_towns.get();
        return *arr;
    }

    town* get(town_id_t id)
    {
        if (id >= _towns.size())
        {
            return nullptr;
        }
        return &_towns[id];
    }

    // 0x00496B6D
    void update()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
        {
            auto ticks = scenario_ticks();
            if (ticks % 8 == 0)
            {
                town_id_t id = (ticks / 8) % 0x7F;
                auto town = get(id);
                if (town != nullptr && !town->empty())
                {
                    companymgr::updating_company_id(company_id::neutral);
                    town->update();
                }
            }
        }
    }

    // 0x0049771C
    void update_labels()
    {
        call(0x0049771C);
    }

    // 0x0049748C
    void update_monthly()
    {
        for (town& currTown : towns())
        {
            if (currTown.name == string_ids::null)
            {
                continue;
            }

            // scroll history
            if (currTown.history_size == 12 * 20)
            {
                memmove(&currTown.history, &currTown.history + 1, 12 * 20 - 1);
            }
            else
            {
                currTown.history_size++;
            }

            int32_t newPop = std::max(currTown.population - currTown.history_min_population, 0);
            int eax = newPop / 50;

            int edx = 0;
            while (eax > 0xFF)
            {
                eax -= 20;
                edx += 1000;
            }

            if (edx != 0)
            {
                currTown.history_min_population += edx;
                int eax2 = edx / 50;

                for (uint8_t i = 0; i < currTown.history_size; i++)
                {
                    currTown.history[i] = std::max(currTown.history[i] - (eax2 & 0xFF), 0);
                }
            }

            currTown.history[currTown.history_size - 1] = eax & 0xFF;
            eax &= ~0xFF; // al = 0;
            uint8_t al = 0;

            uint8_t max = 0;
            for (int i = 0; i < currTown.history_size; i++)
            {
                max = std::max(max, currTown.history[i]);
            }

            edx = currTown.history_min_population;
            while (al <= 0xEB && edx > 0)
            {
                al += 20;
                edx -= 1000;
            }

            edx -= currTown.history_min_population;
            if (edx != 0)
            {
                edx = -edx;
                currTown.history_min_population -= edx;
                edx /= 50;

                for (int i = 0; i < currTown.history_size; i++)
                {
                    currTown.history[i] += edx;
                }
            }

            int16_t ax = -1;
            uint32_t ebx = currTown.unk_198;
            while (ebx != 0)
            {
                int ecx;
                for (ecx = 0; ecx < 32; ecx++)
                {
                    if ((ebx & (1 << ecx)) != 0)
                    {
                        ebx &= ~(1 << ecx);
                        break;
                    }
                }

                ax = std::max(ax, currTown.unk_158[ecx]);
            }

            currTown.build_speed = std::clamp(ax / 100, 1, 3);

            memset(&currTown.unk_158, 0, 20 * sizeof(uint16_t));
        }

        ui::WindowManager::invalidate(ui::WindowType::town);
    }
}
