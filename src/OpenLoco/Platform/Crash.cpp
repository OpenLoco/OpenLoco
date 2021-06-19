#include "Crash.h"
#if defined(USE_BREAKPAD)
#include "../OpenLoco.h"
#include "../Utility/String.hpp"
#include "Platform.h"
#include <ShlObj.h>
#include <client/windows/handler/exception_handler.h>

static bool OnCrash(
    const wchar_t* dumpPath, const wchar_t* miniDumpId, void* context, EXCEPTION_POINTERS* exinfo,
    MDRawAssertionInfo* assertion, bool succeeded)
{
    if (!succeeded)
    {
        constexpr const char* DumpFailedMessage = "Failed to create the dump. Please file an issue with OpenLoco on GitHub and "
                                                  "provide latest save, and provide "
                                                  "information about what you did before the crash occurred.";
        printf("%s\n", DumpFailedMessage);
        MessageBoxA(nullptr, DumpFailedMessage, "OpenLoco", MB_OK | MB_ICONERROR);
        return succeeded;
    }
    wchar_t dumpFilePath[MAX_PATH];
    swprintf_s(dumpFilePath, std::size(dumpFilePath), L"%s\\%s.dmp", dumpPath, miniDumpId);

    std::wstring version = OpenLoco::Utility::toUtf16(OpenLoco::getVersionInfo());

    wchar_t dumpFilePathNew[MAX_PATH];
    swprintf_s(
        dumpFilePathNew, std::size(dumpFilePathNew), L"%s\\%s(%s).dmp", dumpPath, miniDumpId, version.c_str());

    // Try to rename the files
    if (_wrename(dumpFilePath, dumpFilePathNew) == 0)
    {
        std::wcscpy(dumpFilePath, dumpFilePathNew);
    }

    // Log information to output
    wprintf(L"Dump Path: %s\n", dumpPath);
    wprintf(L"Dump File Path: %s\n", dumpFilePath);
    wprintf(L"Dump Id: %s\n", miniDumpId);
    wprintf(L"Version: %s\n", version.c_str());

    constexpr const wchar_t* MessageFormat = L"A crash has occurred and a dump was created at\n%s.\n\nPlease file an issue "
                                             L"with OpenLoco on GitHub and provide "
                                             L"the dump and saved game there.\n\n\nVersion: %s\n\n";
    wchar_t message[MAX_PATH * 2];
    swprintf_s(message, MessageFormat, dumpFilePath, version.c_str());
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

static std::wstring GetDumpDirectory()
{
    auto user_dir = OpenLoco::platform::getUserDirectory();
    auto result = OpenLoco::Utility::toUtf16(user_dir.string());
    return result;
}
#endif // USE_BREAKPAD

// Using non-null pipe name here lets breakpad try setting OOP crash handling
constexpr const wchar_t* PipeName = L"openloco-bpad";

CExceptionHandler crash_init()
{
#if defined(USE_BREAKPAD)
    // Path must exist and be RW!
    auto exHandler = new google_breakpad::ExceptionHandler(
        GetDumpDirectory(), 0, OnCrash, 0, google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpWithDataSegs, PipeName, 0);
    return reinterpret_cast<CExceptionHandler>(exHandler);
#else  // USE_BREAKPAD
    return nullptr;
#endif // USE_BREAKPAD
}
