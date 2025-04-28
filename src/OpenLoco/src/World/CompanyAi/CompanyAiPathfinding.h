#pragma once
#include <sfl/static_vector.hpp>

namespace OpenLoco
{
    struct Company;
    struct AiThought;
}

namespace OpenLoco::CompanyAi
{
    sfl::static_vector<uint8_t, 64> sub_483A7E(Company& company, AiThought& thought);
}
