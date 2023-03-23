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

    void LogSink::enableLevel(Level level)
    {
        const auto bitMask = 1U << static_cast<unsigned>(level);
        _levelMask |= bitMask;
    }

    void LogSink::disableLevel(Level level)
    {
        const auto bitMask = 1U << static_cast<unsigned>(level);
        _levelMask &= ~bitMask;
    }

    bool LogSink::passesLevelFilter(Level level) const noexcept
    {
        const auto bitMask = 1U << static_cast<unsigned>(level);
        return (_levelMask & bitMask) != 0;
    }

    void LogSink::setLevelMask(uint32_t mask)
    {
        _levelMask = mask;
    }

}
