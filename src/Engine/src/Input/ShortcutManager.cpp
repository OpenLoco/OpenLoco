#include "OpenLoco/Engine/Input/ShortcutManager.h"

#include <algorithm>

namespace OpenLoco::Input::ShortcutManager
{
    static ShortcutMap _shortcuts;

    static auto findShortcut(Shortcut id)
    {
        auto it = std::lower_bound(std::begin(_shortcuts), std::end(_shortcuts), id, [](const auto& a, const auto& b) {
            return a.id < b;
        });
        if (it == std::end(_shortcuts))
            return std::end(_shortcuts);
        return it;
    }

    void add(Shortcut id, StringId displayName, const ShortcutAction& action, const char* configName, const char* defaultBinding)
    {
        auto it = findShortcut(id);
        if (it == std::end(_shortcuts) || it->id != id)
        {
            _shortcuts.insert(it, ShortcutEntry{ id, action, displayName, configName, defaultBinding });
        }
        else
        {
            it->id = id;
            it->action = action;
            it->configName = configName;
            it->defaultBinding = defaultBinding;
        }
    }

    void remove(Shortcut id)
    {
        auto it = findShortcut(id);
        if (it == std::end(_shortcuts) || it->id != id)
            return;
        _shortcuts.erase(it);
    }

    void execute(Shortcut id)
    {
        auto it = findShortcut(id);
        if (it == std::end(_shortcuts) || it->id != id)
            return;
        it->action();
    }

    StringId getName(Shortcut id)
    {
        auto it = findShortcut(id);
        if (it == std::end(_shortcuts) || it->id != id)
            return 0xFFFF; // TODO: String-id null

        return it->displayName;
    }

    const ShortcutMap& getList()
    {
        return _shortcuts;
    }

}
