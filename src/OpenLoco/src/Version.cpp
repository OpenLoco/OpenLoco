#include "OpenLoco.h"

// clang-format off

#define OPENLOCO_NAME "OpenLoco"

#if defined(__amd64__) || defined(_M_AMD64)
    #define OPENLOCO_ARCHITECTURE "x86-64"
#elif defined(__i386__) || defined(_M_IX86)
    #define OPENLOCO_ARCHITECTURE "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define OPENLOCO_ARCHITECTURE "AArch64"
#elif defined(__arm__) || defined(_M_ARM)
    #if defined(__ARM_ARCH_7A__)
        #define OPENLOCO_ARCHITECTURE "arm-v7a"
    #else
        #define OPENLOCO_ARCHITECTURE "arm"
    #endif
#endif
#ifndef OPENLOCO_ARCHITECTURE
    #error "OPENLOCO_ARCHITECTURE is undefined. Please add identification."
#endif

#ifdef _WIN32
    #define OPENLOCO_PLATFORM "Windows"
#endif
#if defined(__linux__) && !defined(__ANDROID__)
    #define OPENLOCO_PLATFORM "Linux"
#endif
#if (defined(__APPLE__) && defined(__MACH__))
    #define OPENLOCO_PLATFORM "macOS"
#endif
#ifdef __FreeBSD__
    #define OPENLOCO_PLATFORM "FreeBSD"
#endif
#ifdef __NetBSD__
    #define OPENLOCO_PLATFORM "NetBSD"
#endif
#ifdef __OpenBSD__
    #define OPENLOCO_PLATFORM "OpenBSD"
#endif
#ifndef OPENLOCO_PLATFORM
    #error "OPENLOCO_PLATFORM is undefined. Please add identification."
#endif

namespace OpenLoco
{
    const char kVersion[] = OPENLOCO_NAME ", "
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

    const char kPlatform[] = OPENLOCO_PLATFORM " (" OPENLOCO_ARCHITECTURE ")";
}

// clang-format on
