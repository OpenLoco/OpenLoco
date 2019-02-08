#include "../types.hpp"
#include "Shortcut.h"
#include <cstddef>

namespace openloco::input::ShortcutManager
{
    size_t count();
    void execute(Shortcut s);
    string_id getName(Shortcut s);
}
