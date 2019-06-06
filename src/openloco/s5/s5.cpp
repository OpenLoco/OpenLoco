#include "s5.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::s5
{
    static loco_global<Options, 0x009C8714> _activeOptions;
    static loco_global<Header, 0x009CCA34> _header;
    static loco_global<Options, 0x009CCA54> _previewOptions;

    Options& getOptions()
    {
        return _activeOptions;
    }

    Options& getPreviewOptions()
    {
        return _previewOptions;
    }
}
