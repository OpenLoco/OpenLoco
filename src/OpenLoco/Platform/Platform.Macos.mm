#if defined(__APPLE__) && defined(__MACH__)

#include "Platform.h"
#include <limits.h>
#include <mach-o/dyld.h>

#import <Cocoa/Cocoa.h>

namespace OpenLoco::platform
{
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

    fs::path promptDirectory(const std::string &title)
    {
        @autoreleasepool
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.canChooseFiles = false;
            panel.canChooseDirectories = true;
            panel.allowsMultipleSelection = false;
            if ([panel runModal] == NSModalResponseOK)
            {
                NSString* selectedPath = panel.URL.path;
                const char* path = selectedPath.UTF8String;
                return path;
            } else {
                return "";
            }
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
}

#endif
