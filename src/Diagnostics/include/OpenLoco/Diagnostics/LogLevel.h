#pragma once

#include <cstdint>

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

}
