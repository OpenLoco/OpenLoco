#pragma once

namespace OpenLoco
{
    struct Company;
    struct AiThought;
}

namespace OpenLoco::CompanyAi
{
    bool aiPathfind(Company& company, AiThought& thought);
}
