#include "town.h"

namespace openloco
{
    bool town::empty() const
    {
        return var_00 != -1;
    }
}
