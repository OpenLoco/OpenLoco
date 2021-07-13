#include "../Types.hpp"
#include "Shortcut.h"
#include <cstddef>

namespace OpenLoco::Input::ShortcutManager
{
    size_t count();
    void execute(Shortcut s);
    string_id getName(Shortcut s);
}
