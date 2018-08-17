#ifndef _WIN32

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
std::string openloco::platform::prompt_directory(const std::string& title)
{
    std::string input;
    std::cout << "Type your Locomotion path: ";
    std::cin >> input;
    return input;
}
#endif

#endif
