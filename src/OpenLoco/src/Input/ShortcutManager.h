#pragma once

#include "../Types.hpp"
#include "Shortcut.h"
#include <array>
#include <cstddef>

namespace OpenLoco::Input::ShortcutManager
{
    static constexpr size_t kCount = 47;

    void execute(Shortcut s);
    string_id getName(Shortcut s);

    struct KeyboardShortcut
    {
        void (*function)();
        string_id displayName;
        const char* configName;
        const char* defaultBinding;
    };

    const std::array<const KeyboardShortcut, kCount>& getList();
}
