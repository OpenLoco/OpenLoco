#pragma once

#include "company.h"
#include <array>
#include <cstddef>

namespace openloco
{
    constexpr size_t max_companies = 15;

    class companymanager
    {
    public:
        companymanager() = default;
        companymanager(const companymanager&) = delete;

        company_id_t updating_company_id() const;
        void updating_company_id(company_id_t id);

        std::array<company, max_companies>& companies();
        company* get(company_id_t id);
        company_id_t get_controlling_id() const;
        void update();

    private:
        void produce_companies();
        void sub_42F9AC();
    };

    extern companymanager g_companymgr;
}
