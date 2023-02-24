#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenLoco::Input
{
    enum class Shortcut : uint32_t;
}

namespace OpenLoco::Input::ShortcutManager
{
    using string_id = uint16_t;

    struct KeyboardShortcut
    {
        Shortcut id;
        void (*function)();
        string_id displayName;
        const char* configName;
        const char* defaultBinding;
    };

    using ShortcutMap = std::vector<KeyboardShortcut>;

    void add(Shortcut id, string_id displayName, void (*function)(), const char* configName, const char* defaultBinding);

    void remove(Shortcut id);

    void execute(Shortcut s);

    string_id getName(Shortcut s);

    const ShortcutMap& getList();
}
