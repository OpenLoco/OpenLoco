#include "industry.h"

namespace openloco
{
    bool industry::empty() const
    {
        return name == string_ids::null;
    }
}
