#pragma once

#include <string>

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
#else
    #error "OPENLOCO_ARCHITECTURE is undefined. Please add identification."
#endif

#ifdef _WIN32
    #define OPENLOCO_PLATFORM "Windows"
#elif defined(__linux__) && !defined(__ANDROID__)
    #define OPENLOCO_PLATFORM "Linux"
#elif (defined(__APPLE__) && defined(__MACH__))
    #define OPENLOCO_PLATFORM "macOS"
#elif defined(__FreeBSD__)
    #define OPENLOCO_PLATFORM "FreeBSD"
#elif defined(__NetBSD__)
    #define OPENLOCO_PLATFORM "NetBSD"
#elif defined(__OpenBSD__)
    #define OPENLOCO_PLATFORM "OpenBSD"
#else
    #error "OPENLOCO_PLATFORM is undefined. Please add identification."
#endif

namespace OpenLoco::Version
{
#ifndef OPENLOCO_VERSION_TAG
#    define OPENLOCO_VERSION_TAG "unknown"
#endif

#ifndef OPENLOCO_BRANCH
#    define OPENLOCO_BRANCH "unknown"
#endif

#ifndef OPENLOCO_COMMIT_SHA1_SHORT
#    define OPENLOCO_COMMIT_SHA1_SHORT "unknown"
#endif

    constexpr const char* kVersionTag = OPENLOCO_VERSION_TAG;
    constexpr const char* kBranch = OPENLOCO_BRANCH;
    constexpr const char* kCommitSha1Short = OPENLOCO_COMMIT_SHA1_SHORT;

    static const char kVersion[] = OPENLOCO_NAME ", "
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

    static const char kPlatform[] = OPENLOCO_PLATFORM " (" OPENLOCO_ARCHITECTURE ")";
    // clang-format on

    inline std::string getVersionInfo()
    {
        return kVersion;
    }

    inline std::string getPlatformInfo()
    {
        return kPlatform;
    }
}
