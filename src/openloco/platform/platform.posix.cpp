#ifndef _WIN32

#include "../console.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "platform.h"
#include <iostream>
#include <pwd.h>
#include <time.h>

#ifdef __linux__
#include <sys/types.h>
#include <unistd.h>
#endif

int main(int argc, const char** argv)
{
    openloco::interop::load_sections();
    openloco::lpCmdLine((char*)argv[0]);
    openloco::main();
    return 0;
}

uint32_t openloco::platform::get_time()
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_nsec / 1000000;
}

#if !(defined(__APPLE__) && defined(__MACH__))
static std::string GetEnvironmentVariable(const std::string& name)
{
    auto result = getenv(name.c_str());
    return result == nullptr ? std::string() : result;
}

static fs::path get_home_directory()
{
    auto pw = getpwuid(getuid());
    if (pw != nullptr)
    {
        return pw->pw_dir;
    }
    else
    {
        return GetEnvironmentVariable("HOME");
    }
}

fs::path openloco::platform::get_user_directory()
{
    auto path = fs::path(GetEnvironmentVariable("XDG_CONFIG_HOME"));
    if (path.empty())
    {
        path = get_home_directory();
        if (path.empty())
        {
            path = "/";
        }
        else
        {
            path = path / fs::path(".config");
        }
    }
    return path / fs::path("OpenLoco");
}
#endif

#if !(defined(__APPLE__) && defined(__MACH__))
fs::path openloco::platform::GetCurrentExecutablePath()
{
    char exePath[PATH_MAX] = { 0 };
#ifdef __linux__
    auto bytesRead = readlink("/proc/self/exe", exePath, sizeof(exePath));
    if (bytesRead == -1)
    {
        console::error("failed to read /proc/self/exe");
    }
#elif defined(__FreeBSD__)
    const int32_t mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    auto exeLen = sizeof(exePath);
    if (sysctl(mib, 4, exePath, &exeLen, nullptr, 0) == -1)
    {
        console::error("failed to get process path");
    }
#elif defined(__OpenBSD__)
    // There is no way to get the path name of a running executable.
    // If you are not using the port or package, you may have to change this line!
    strlcpy(exePath, "/usr/local/bin/", sizeof(exePath));
#else
#error "Platform does not support full path exe retrieval"
#endif
    return exePath;
}

std::string openloco::platform::prompt_directory(const std::string& title)
{
    std::string input;
    std::cout << "Type your Locomotion path: ";
    std::cin >> input;
    return input;
}
#endif

#endif
