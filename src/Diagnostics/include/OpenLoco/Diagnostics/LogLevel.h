#pragma once

#include <cstdint>
#include <string_view>

namespace OpenLoco::Diagnostics::Logging
{
    enum class Level : uint32_t
    {
        info,
        warning,
        error,
        verbose,
        all,
    };

    using LevelMask = uint32_t;

    template<typename... TArgs>
    constexpr LevelMask getLevelMask(TArgs&&... args) noexcept
    {
        LevelMask mask{};
        const auto maskForLevel = [](Level level) {
            if (level == Level::all)
                return ~0U;
            else
                return 1U << static_cast<LevelMask>(level);
        };
        ((mask = mask | maskForLevel(args)), ...);
        return mask;
    }

    constexpr LevelMask getLevelMaskFromName(std::string_view name)
    {
        if (name == "info")
            return getLevelMask(Level::info);
        else if (name == "warning")
            return getLevelMask(Level::warning);
        else if (name == "error")
            return getLevelMask(Level::error);
        else if (name == "verbose")
            return getLevelMask(Level::verbose);
        else if (name == "all")
            return getLevelMask(Level::all);
        else
            return 0;
    }

    constexpr std::string_view getLevelPrefix(Level level)
    {
        switch (level)
        {
            case Level::info:
                return "[INF] ";
            case Level::warning:
                return "[WRN] ";
            case Level::error:
                return "[ERR] ";
            case Level::verbose:
                return "[VER] ";
            default:
                break;
        }
        return "[INVALID] ";
    }

}
