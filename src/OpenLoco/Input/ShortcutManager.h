#include "../Types.hpp"
#include "Shortcut.h"
#include <array>
#include <cstddef>
#include <string>

namespace OpenLoco::Input::ShortcutManager
{
    static constexpr size_t count = 35;

    void execute(Shortcut s);
    string_id getName(Shortcut s);

    struct KeyboardShortcut
    {
        void (*function)();
        string_id displayName;
        std::string configName;
        std::string defaultBinding;
    };

    // clang-format off
    std::array<const KeyboardShortcut, count>& getList();
}
