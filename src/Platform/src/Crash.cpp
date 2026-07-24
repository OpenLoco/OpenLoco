#include "Crash.h"

#if defined(USE_CRASHPAD)
#include "Platform.h"
#include <OpenLoco/Utility/String.hpp>
#include <base/files/file_path.h>
#include <client/crashpad_client.h>
#include <windows.h>
#include <map>
#include <vector>
#endif

namespace OpenLoco::CrashHandler
{
#if defined(USE_CRASHPAD)
    [[maybe_unused]] static std::wstring getDumpDirectory()
    {
        auto crashDir = Platform::getUserDirectory() / "crashes";
        if (!fs::exists(crashDir))
        {
            fs::create_directories(crashDir);
        }
        return crashDir.wstring();
    }

    [[maybe_unused]] static std::wstring getHandlerPath()
    {
        wchar_t modulePath[MAX_PATH] = {};
        auto length = GetModuleFileNameW(nullptr, modulePath, static_cast<DWORD>(std::size(modulePath)));
        if (length == 0 || length == std::size(modulePath))
        {
            return {};
        }
        fs::path exePath(modulePath);
        return (exePath.parent_path() / L"crashpad_handler.exe").wstring();
    }

#endif // USE_CRASHPAD

    Handle init([[maybe_unused]] const AppInfo& appInfo)
    {
#if !defined(DEBUG) && defined(USE_CRASHPAD)
        const auto handlerPath = getHandlerPath();
        const auto crashDir = getDumpDirectory();

        if (handlerPath.empty() || !fs::exists(handlerPath))
        {
            wprintf(L"Crashpad handler not found at '%s'\n", handlerPath.c_str());
            return nullptr;
        }

        auto exHandler = new crashpad::CrashpadClient();

        std::map<std::string, std::string> annotations = {
            { "prod", appInfo.name },
            { "ver", appInfo.version },
        };

        const bool started = exHandler->StartHandler(
            base::FilePath(handlerPath),
            base::FilePath(crashDir),
            base::FilePath(crashDir),
            std::string(),
            annotations,
            std::vector<std::string>(),
            true,
            true);

        if (!started || !exHandler->WaitForHandlerStart(INFINITE))
        {
            wprintf(L"Crashpad failed to start from '%s'\n", handlerPath.c_str());
            delete exHandler;
            return nullptr;
        }

        return exHandler;
#else
        return nullptr;
#endif
    }

    void shutdown([[maybe_unused]] Handle exHandler)
    {
#if !defined(DEBUG) && defined(USE_CRASHPAD)
        if (exHandler == nullptr)
        {
            return;
        }

        delete static_cast<crashpad::CrashpadClient*>(exHandler);
#endif
    }
}
