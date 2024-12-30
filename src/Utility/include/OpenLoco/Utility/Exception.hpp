#pragma once

#include <stdexcept>
#include <source_location>
#include <iostream>

namespace OpenLoco
{
    class NotImplementedException : public std::logic_error
    {
    public:
        NotImplementedException(std::source_location location)
            : std::logic_error("function or case not implemented yet")
        {
            std::clog << "at file: " << location.file_name() << "(" << location.line() << ":" << location.column() << ")" << std::endl;
        }
    };
}
