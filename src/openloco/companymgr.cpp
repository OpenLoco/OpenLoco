#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"

using namespace openloco::interop;

namespace openloco::companymgr
{
    static loco_global<company_id_t[2], 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00525FCB> _byte_525FCB;
    static loco_global<uint8_t, 0x00526214> _company_competition_delay;
    static loco_global<uint8_t, 0x00525FB7> _company_max_competing;
    static loco_global<uint8_t, 0x00525E3C> _byte_525E3C;
    static loco_global<uint8_t, 0x00525E3D> _byte_525E3D;
    static loco_global<company[max_companies], 0x00531784> _companies;
    static loco_global<uint8_t[max_companies + 1], 0x009C645C> _company_colours;
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;

    static void produce_companies();

    company_id_t updating_company_id()
    {
        return _updating_company_id;
    }

    void updating_company_id(company_id_t id)
    {
        _updating_company_id = id;
    }

    std::array<company, max_companies>& companies()
    {
        auto arr = (std::array<company, max_companies>*)_companies.get();
        return *arr;
    }

    company* get(company_id_t id)
    {
        auto index = id;
        if (index < _companies.size())
        {
            return &_companies[index];
        }
        return nullptr;
    }

    company_id_t get_controlling_id()
    {
        return _player_company[0];
    }

    uint8_t get_company_colour(company_id_t id)
    {
        return _company_colours[id];
    }

    uint8_t get_player_company_colour()
    {
        return _company_colours[_player_company[0]];
    }

    // 0x00430319
    void update()
    {
        if (!is_editor_mode())
        {
            company_id_t id = scenario_ticks() & 0x0F;
            auto company = get(id);
            if (company != nullptr && !is_player_company(id) && !company->empty())
            {
                updating_company_id(id);
                company->ai_think();
            }

            _byte_525FCB++;
            if (_byte_525FCB >= 192)
            {
                _byte_525FCB = 0;
                produce_companies();
            }
        }
    }

    static void sub_42F9AC()
    {
        call(0x0042F9AC);
    }

    // 0x004306D1
    static void produce_companies()
    {
        if (_company_competition_delay == 0 && _company_max_competing != 0)
        {
            int32_t companies_active = 0;
            for (const auto& company : companies())
            {
                auto id = company.id();
                if (!company.empty() && id != _byte_525E3C && id != _byte_525E3D)
                {
                    companies_active++;
                }
            }

            auto& prng = gprng();

            if (prng.rand_next(16) == 0)
            {
                if (prng.rand_next(_company_max_competing) + 1 > companies_active)
                {
                    // Creates new company.
                    sub_42F9AC();
                }
            }
        }
    }

    company* getOpponent()
    {
        return &_companies[_player_company[1]];
    }
}
