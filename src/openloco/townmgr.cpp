#include "townmgr.h"
#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include "ui/WindowManager.h"
#include "utility/numeric.hpp"

using namespace openloco::interop;

namespace openloco::townmgr
{
    static loco_global<town[max_towns], 0x005B825C> _towns;

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
            if (currTown.empty())
                continue;

            // Scroll history
            if (currTown.history_size == std::size(currTown.history))
            {
                for (size_t i = 0; i < std::size(currTown.history) - 1; i++)
                    currTown.history[i] = currTown.history[i + 1];
            }
            else
                currTown.history_size++;

            // Compute population growth.
            uint32_t popSteps = std::max<int32_t>(currTown.population - currTown.history_min_population, 0) / 50;
            uint32_t popGrowth = 0;
            while (popSteps > 255)
            {
                popSteps -= 20;
                popGrowth += 1000;
            }

            // Any population growth to account for?
            if (popGrowth != 0)
            {
                currTown.history_min_population += popGrowth;

                uint8_t offset = (popGrowth / 50) & 0xFF;
                for (uint8_t i = 0; i < currTown.history_size; i++)
                {
                    int16_t newHistory = currTown.history[i] - offset;
                    currTown.history[i] = newHistory >= 0 ? static_cast<uint8_t>(newHistory) : 0;
                }
            }

            // Write new history point.
            currTown.history[currTown.history_size - 1] = popSteps & 0xFF;

            // Find historical maximum population.
            uint8_t maxPopulation = 0;
            for (int i = 0; i < currTown.history_size; i++)
                maxPopulation = std::max(maxPopulation, currTown.history[i]);

            int32_t popOffset = currTown.history_min_population;
            while (maxPopulation <= 235 && popOffset > 0)
            {
                maxPopulation += 20;
                popOffset -= 1000;
            }

            popOffset -= currTown.history_min_population;
            if (popOffset != 0)
            {
                popOffset = -popOffset;
                currTown.history_min_population -= popOffset;
                popOffset /= 50;

                for (int i = 0; i < currTown.history_size; i++)
                    currTown.history[i] += popOffset;
            }

            // Work towards computing new build speed.
            int16_t maxCargoDelivered = -1;
            uint32_t cargoFlags = currTown.cargo_influence_flags;
            while (cargoFlags != 0)
            {
                uint32_t cargoId = utility::bitscanforward(cargoFlags);
                cargoFlags &= ~(1 << cargoId);

                maxCargoDelivered = std::max(maxCargoDelivered, currTown.monthly_cargo_delivered[cargoId]);
            }

            // Compute build speed (1=slow build speed, 4=fast build speed)
            currTown.build_speed = std::clamp((maxCargoDelivered / 100) + 1, 1, 4);

            // Reset all monthly_cargo_delivered intermediaries to zero.
            memset(&currTown.monthly_cargo_delivered, 0, sizeof(currTown.monthly_cargo_delivered));
        }

        ui::WindowManager::invalidate(ui::WindowType::town);
    }
}
