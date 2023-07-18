#pragma once

#include <string>

namespace OpenLoco::CrashHandler
{
    using Handle = void*;

    struct AppInfo
    {
        std::string name;
        std::string version;
    };

    Handle init(const AppInfo& appInfo);
    void shutdown(Handle handler);

}
