#pragma once

#include <stdexcept>

namespace OpenLoco
{
    class NotImplementedException : public std::logic_error
    {
    public:
        NotImplementedException()
            : std::logic_error("function or case not implemented yet.")
        {
        }
    };
}
