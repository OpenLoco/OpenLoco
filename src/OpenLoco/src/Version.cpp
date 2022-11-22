#include "OpenLoco.h"

#define NAME "OpenLoco"

namespace OpenLoco
{
    const char version[] = NAME ", "
#ifdef OPENLOCO_VERSION_TAG
        OPENLOCO_VERSION_TAG
#else
#error "OPENLOCO_VERSION_TAG Not set. Rerun CMake."
#endif
#if defined(OPENLOCO_BRANCH) || defined(OPENLOCO_COMMIT_SHA1_SHORT) || !defined(NDEBUG)
                                " ("
#if defined(OPENLOCO_BRANCH) && defined(OPENLOCO_COMMIT_SHA1_SHORT)
        OPENLOCO_COMMIT_SHA1_SHORT " on " OPENLOCO_BRANCH
#elif defined(OPENLOCO_COMMIT_SHA1_SHORT)
        OPENLOCO_COMMIT_SHA1_SHORT
#elif defined(OPENLOCO_BRANCH)
        OPENLOCO_BRANCH
#endif
#ifndef NDEBUG
                                ", DEBUG"
#endif
                                ")"
#endif
        ;
}
