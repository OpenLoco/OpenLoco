#if defined(__APPLE__) && defined(__MACH__)

#include "Platform.h"
#include <cstdlib>
#include <limits.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#import <Cocoa/Cocoa.h>

namespace OpenLoco::Platform
{
    void initialise()
    {
    }

    uint32_t getTime()
    {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return (spec.tv_sec * 1000) + spec.tv_nsec / 1000000;
    }

    std::vector<fs::path> getDrives()
    {
        return {};
    }

    fs::path getUserDirectory()
    {
        @autoreleasepool
        {
            NSFileManager* filemanager = [NSFileManager defaultManager];
            NSURL* url = [[filemanager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject];
            url = [url URLByAppendingPathComponent:@"OpenLoco"];
            return url.path.UTF8String;
        }
    }

    fs::path GetCurrentExecutablePath()
    {
        char exePath[PATH_MAX];
        uint32_t size = PATH_MAX;
        int result = _NSGetExecutablePath(exePath, &size);
        if (result == 0)
        {
            return exePath;
        }
        else
        {
            return fs::path();
        }
    }

    fs::path GetBundlePath()
    {
        @autoreleasepool
        {
            NSBundle* bundle = [NSBundle mainBundle];
            if (bundle)
            {
                auto resources = bundle.resourcePath.UTF8String;
                if (fs::exists(resources))
                {
                    return resources;
                }
            }
            return fs::path();
        }
    }

    std::string getEnvironmentVariable(const std::string& name)
    {
        auto result = std::getenv(name.c_str());
        return result == nullptr ? std::string() : result;
    }

    std::vector<fs::path> getLocoInstallSearchPaths()
    {
        // TODO: Implement me. Locomotion is a Windows game, so any install here is made by
        // a compatibility layer such as CrossOver or Whisky, in a location of its choosing.
        return {};
    }

    bool isRunningInWine()
    {
        return false;
    }

    bool isStdOutRedirected()
    {
        // TODO: Implement me
        return false;
    }

    static bool hasTerminalVT100SupportImpl()
    {
        // TODO: Implement me.
        return false;
    }

    bool hasTerminalVT100Support()
    {
        static bool hasVT100Support = hasTerminalVT100SupportImpl();
        return hasVT100Support;
    }

    bool enableVT100TerminalMode()
    {
        if (!isStdOutRedirected())
            return false;

        if (!hasTerminalVT100Support())
            return false;

        return true;
    }

    std::vector<std::string> getCmdLineVector(int argc, const char** argv)
    {
        std::vector<std::string> argvStrs;
        argvStrs.resize(argc);
        for (auto i = 0; i < argc; ++i)
        {
            argvStrs[i] = argv[i];
        }
        return argvStrs;
    }

    bool lockSingleInstance()
    {
        // TODO: stub!
        return true;
    }
}

#endif
