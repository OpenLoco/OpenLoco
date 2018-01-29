
#if defined(__APPLE__) && defined(__MACH__)

#include "platform.h"

#import <Cocoa/Cocoa.h>

fs::path openloco::platform::get_user_directory()
{
    @autoreleasepool
    {
        NSFileManager * filemanager = [NSFileManager defaultManager];
        NSURL *url = [[filemanager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject];
        url = [url URLByAppendingPathComponent:@"OpenLoco"];
        return url.path.UTF8String;
    }
}

std::string openloco::platform::prompt_directory(const std::string &title)
{

    @autoreleasepool
    {
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = false;
        panel.canChooseDirectories = true;
        panel.allowsMultipleSelection = false;
        if ([panel runModal] == NSModalResponseOK)
        {
            NSString *selectedPath = panel.URL.path;
            const char *path = selectedPath.UTF8String;
            return path;
        } else {
            return "";
        }
    }
}

#endif
