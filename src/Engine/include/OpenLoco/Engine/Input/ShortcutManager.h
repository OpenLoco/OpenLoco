#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace OpenLoco
{
    // TODO: Remove this when Localisation/StringTable/StringManager is moved to the engine.
    using StringId = uint16_t;
}

namespace OpenLoco::Input
{
    enum class Shortcut : uint32_t;
}

namespace OpenLoco::Input::ShortcutManager
{
    using ShortcutAction = std::function<void()>;

    struct ShortcutEntry
    {
        Shortcut id;
        ShortcutAction action;
        StringId displayName;
        const char* configName;
        const char* defaultBinding;
    };

    using ShortcutMap = std::vector<ShortcutEntry>;

    void add(Shortcut id, StringId displayName, const ShortcutAction& action, const char* configName, const char* defaultBinding);

    void remove(Shortcut id);

    void execute(Shortcut s);

    StringId getName(Shortcut s);

    const ShortcutMap& getList();
}
