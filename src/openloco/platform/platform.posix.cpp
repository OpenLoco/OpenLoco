#ifndef _WIN32

#include "../openloco.h"
#include "platform.h"
#include <pwd.h>
#include <time.h>

#ifdef __linux__
#include <sys/types.h>
#include <unistd.h>
#endif

int main(int argc, const char** argv)
{
    openloco::main();
    return 0;
}

static std::string GetEnvironmentVariable(const std::string& name)
{
    return getenv(name.c_str());
}

uint32_t openloco::platform::get_time()
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_nsec / 1000000;
}

fs::path openloco::platform::get_user_directory()
{
    std::string path;
    auto pw = getpwuid(getuid());
    if (pw != nullptr)
    {
        path = pw->pw_dir;
    }
    else
    {
        path = GetEnvironmentVariable("HOME");
    }
    if (path.empty())
    {
        path = "/";
    }
    return path;
}

#if !(defined(__APPLE__) && defined(__MACH__))
std::string openloco::platform::prompt_directory(const std::string& title)
{
    return "/";
}
#endif

#endif
