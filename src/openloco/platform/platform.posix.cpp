#include <time.h>
#include <pwd.h>
#include "platform.h"



static std::string GetEnvironmentVariable(const std::string &name)
{
    return getenv(name.c_str());
}

uint32_t openloco::platform::get_time() {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    return spec.tv_nsec/1000000;
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