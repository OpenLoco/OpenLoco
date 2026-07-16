#pragma once

#include "Types.hpp"
#include <cstddef>
#include <cstdint>

namespace OpenLoco::Config
{
    struct KeyboardShortcut;
}

namespace OpenLoco::Input
{
    enum class Shortcut : uint32_t;

    namespace ShortcutFormatter
    {
        constexpr size_t kShortcutBufferSize = 128;

        struct Binding
        {
            StringId modifierStringId;
            StringId keyStringId;
            const char* keyString;
            bool isBound;
        };

        Binding getBinding(const Config::KeyboardShortcut& shortcut, char* buffer, size_t bufferLength);
        Binding getBinding(Shortcut shortcut, char* buffer, size_t bufferLength);
    }
}
