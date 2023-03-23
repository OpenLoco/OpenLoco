#include "OpenLoco/Diagnostics/LogSink.h"
#include <cassert>

namespace OpenLoco::Diagnostics::Logging
{

    void LogSink::setWriteTimestamps(bool value)
    {
        _writeTimestamps = value;
    }

    bool LogSink::getWriteTimestamps() const noexcept
    {
        return _writeTimestamps;
    }

    int LogSink::getIntendSize() const noexcept
    {
        return _intendSize;
    }

    void LogSink::setIntendSize(int size)
    {
        _intendSize = size;
        assert(_intendSize >= 0);
    }

    void LogSink::incrementIntendSize()
    {
        _intendSize++;
    }

    void LogSink::decrementIntendSize()
    {
        _intendSize--;
        assert(_intendSize >= 0);
    }

}
