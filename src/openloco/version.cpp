#include "openloco.h"

// clang-format off
#define NAME            "OpenLoco"
#define VERSION         "18.02.2"
#define VERSION_INFO    NAME ", v" VERSION
// clang-format on

namespace openloco
{
    const char version[] = VERSION_INFO
#ifdef OPENLOCO_BRANCH
        "-" OPENLOCO_BRANCH
#endif
#ifdef OPENLOCO_COMMIT_SHA1_SHORT
        " build " OPENLOCO_COMMIT_SHA1_SHORT
#endif
#ifndef NDEBUG
        " (DEBUG)"
#endif
        ;
}
