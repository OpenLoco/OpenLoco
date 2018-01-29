#include "industrymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"

using namespace openloco::interop;

namespace openloco::industrymgr
{
    static loco_global_array<industry, max_industries, 0x005C455C> _industries;

    std::array<industry, max_industries>& industries()
    {
        auto arr = (std::array<industry, max_industries>*)_industries.get();
        return *arr;
    }

    industry* get(industry_id_t id)
    {
        if (id >= _industries.size())
        {
            return nullptr;
        }
        return &_industries[id];
    }

    // 0x00453234
    void update()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
        {
            addr<0x009C68EB, uint8_t>() = 15;
            for (auto& industry : industries())
            {
                if (!industry.empty())
                {
                    industry.update();
                }
            }
        }

        call(0x00453234);
    }
}
