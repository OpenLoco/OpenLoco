#include "Crash.h"

#if defined(USE_BREAKPAD)
#include "Platform.h"
#include <OpenLoco/Utility/String.hpp>
#include <ShlObj.h>
#include <client/windows/handler/exception_handler.h>
#endif

namespace OpenLoco::CrashHandler
{
#if defined(USE_BREAKPAD)
    static AppInfo _appInfo;

    [[maybe_unused]] static bool onCrash(
        const wchar_t* dumpPath, const wchar_t* miniDumpId,
        [[maybe_unused]] void* context,
        [[maybe_unused]] EXCEPTION_POINTERS* exinfo,
        [[maybe_unused]] MDRawAssertionInfo* assertion,
        bool succeeded)
    {
        if (!succeeded)
        {
            constexpr const char* dumpFailedMessage = "Failed to create the dump. Please file an issue with OpenLoco on GitHub and "
                                                      "provide latest save, and provide "
                                                      "information about what you did before the crash occurred.";
            printf("%s\n", dumpFailedMessage);
            MessageBoxA(nullptr, dumpFailedMessage, "OpenLoco", MB_OK | MB_ICONERROR);
            return succeeded;
        }
        wchar_t dumpFilePath[MAX_PATH];
        swprintf_s(dumpFilePath, std::size(dumpFilePath), L"%s\\%s.dmp", dumpPath, miniDumpId);

        std::wstring version = Utility::toUtf16(_appInfo.version);

        wchar_t dumpFilePathNew[MAX_PATH];
        swprintf_s(
            dumpFilePathNew, std::size(dumpFilePathNew), L"%s\\%s(%s).dmp", dumpPath, miniDumpId, version.c_str());

        // Try to rename the files
        if (_wrename(dumpFilePath, dumpFilePathNew) == 0)
        {
            wcscpy_s(dumpFilePath, dumpFilePathNew);
        }

        // Log information to output
        wprintf(L"Dump Path: %s\n", dumpPath);
        wprintf(L"Dump File Path: %s\n", dumpFilePath);
        wprintf(L"Dump Id: %s\n", miniDumpId);
        wprintf(L"Version: %s\n", version.c_str());

        constexpr const wchar_t* MessageFormat = L"A crash has occurred and a dump was created at\n%s.\n\nPlease file an issue "
                                                 L"with %S on GitHub and provide "
                                                 L"the dump and most recently saved game there.\n\n\nVersion: %s\n\n";
        wchar_t message[MAX_PATH * 2];
        swprintf_s(message, MessageFormat, dumpFilePath, _appInfo.name.c_str(), version.c_str());
        MessageBoxW(nullptr, message, L"OpenLoco", MB_OK | MB_ICONERROR);

        HRESULT coInitializeResult = CoInitialize(nullptr);
        if (SUCCEEDED(coInitializeResult))
        {
            LPITEMIDLIST pidl = ILCreateFromPathW(dumpPath);
            LPITEMIDLIST files[6];
            uint32_t numFiles = 0;

            files[numFiles++] = ILCreateFromPathW(dumpFilePath);
            if (pidl != nullptr)
            {
                SHOpenFolderAndSelectItems(pidl, numFiles, (LPCITEMIDLIST*)files, 0);
                ILFree(pidl);
                for (uint32_t i = 0; i < numFiles; i++)
                {
                    ILFree(files[i]);
                }
            }
            CoUninitialize();
        }
        return succeeded;
    }

    [[maybe_unused]] static std::wstring getDumpDirectory()
    {
        auto crashDir = Platform::getUserDirectory() / "crashes";
        if (!fs::exists(crashDir))
        {
            fs::create_directories(crashDir);
        }
        return crashDir.wstring();
    }

#endif // USE_BREAKPAD

    Handle init([[maybe_unused]] const AppInfo& appInfo)
    {
#if !defined(DEBUG) && defined(USE_BREAKPAD)
        _appInfo = appInfo;

        const auto pipeName = Utility::toUtf16(appInfo.name + "_breakpad");

        // Path must exist and be RW!
        auto exHandler = new google_breakpad::ExceptionHandler(
            getDumpDirectory(), 0, onCrash, 0, google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpWithDataSegs, pipeName.c_str(), 0);
        return exHandler;
#else
        return nullptr;
#endif
    }

    void shutdown([[maybe_unused]] Handle exHandler)
    {
#if !defined(DEBUG) && defined(USE_BREAKPAD)
        if (exHandler == nullptr)
            return;

        delete static_cast<google_breakpad::ExceptionHandler*>(exHandler);
#endif
    }
}
