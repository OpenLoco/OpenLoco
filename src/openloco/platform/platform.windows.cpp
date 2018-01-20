#ifdef _WIN32

#include "platform.h"


uint32_t openloco::platform::get_time() {
    return timeGetTime();
}


fs::path  openloco::platform::get_user_directory()
{
    auto result = fs::path();
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &path)))
    {
        result = fs::path(path) / "OpenLoco";
    }
    CoTaskMemFree(path);
    return result;
}

#endif
