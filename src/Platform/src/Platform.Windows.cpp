#ifdef _WIN32

#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1

#include "Platform.h"
#include <cstdlib>
#include <io.h>
#include <iostream>
#include <tuple>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
// Windows headers are quite sensitive to the include order.
#include <shlobj.h>
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
// clang-format on

#include <OpenLoco/Utility/String.hpp>

namespace OpenLoco::Platform
{
    uint32_t getTime()
    {
        return timeGetTime();
    }

    fs::path getUserDirectory()
    {
        auto result = fs::path{};
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, path)))
        {
            result = fs::path(path) / "OpenLoco";
        }
        return result;
    }

    static std::wstring SHGetPathFromIDListLongPath(LPCITEMIDLIST pidl)
    {
        std::wstring pszPath(MAX_PATH, 0);
        while (!SHGetPathFromIDListW(pidl, &pszPath[0]))
        {
            if (pszPath.size() >= SHRT_MAX)
            {
                // Clearly not succeeding at all, bail
                return std::wstring();
            }
            pszPath.resize(pszPath.size() * 2);
        }

        auto nullBytePos = pszPath.find(L'\0');
        if (nullBytePos != std::wstring::npos)
            pszPath.resize(nullBytePos);

        return pszPath;
    }

    fs::path promptDirectory(const std::string& title, void* hwnd)
    {
        fs::path result;

        // Initialize COM and get a pointer to the shell memory allocator
        LPMALLOC lpMalloc;
        if (SUCCEEDED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)) && SUCCEEDED(SHGetMalloc(&lpMalloc)))
        {
            auto titleW = Utility::toUtf16(title);
            BROWSEINFOW bi{};
            bi.lpszTitle = titleW.c_str();
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl != nullptr)
            {
                result = fs::path(SHGetPathFromIDListLongPath(pidl).c_str());
            }
            CoTaskMemFree(pidl);
        }
        else
        {
            std::cerr << "Error opening directory browse window";
        }
        CoUninitialize();

        // SHBrowseForFolderW might minimize the main window,
        // so make sure that it's visible again.
        if (hwnd != nullptr)
        {
            ShowWindow(static_cast<HWND>(hwnd), SW_RESTORE);
        }

        return result;
    }

    static fs::path WIN32_GetModuleFileNameW(HMODULE hModule)
    {
        uint32_t wExePathCapacity = MAX_PATH;
        std::unique_ptr<wchar_t[]> wExePath;
        uint32_t size;
        do
        {
            wExePathCapacity *= 2;
            wExePath = std::make_unique<wchar_t[]>(wExePathCapacity);
            size = GetModuleFileNameW(hModule, wExePath.get(), wExePathCapacity);
        } while (size >= wExePathCapacity);
        return fs::path(wExePath.get());
    }

    fs::path getCurrentExecutablePath()
    {
        return WIN32_GetModuleFileNameW(nullptr);
    }

    std::vector<fs::path> getDrives()
    {
        char drive[4] = { 'A', ':', '\0' };
        std::vector<fs::path> drives;
        auto driveMask = GetLogicalDrives();
        for (auto i = 0; i < 26; i++)
        {
            if (driveMask & (1 << i))
            {
                drive[0] = 'A' + i;
                std::error_code ec;
                // Remove unaccessable drives (if error it is unaccessable)
                if (fs::is_directory(drive, ec))
                {
                    drives.push_back(drive);
                }
            }
        }
        return drives;
    }

    std::string getEnvironmentVariable(const std::string& name)
    {
        auto result = std::getenv(name.c_str());
        return result == nullptr ? std::string() : result;
    }

    bool isRunningInWine()
    {
        HMODULE ntdllMod = GetModuleHandleW(L"ntdll.dll");

        if (ntdllMod && GetProcAddress(ntdllMod, "wine_get_version"))
        {
            return true;
        }
        return false;
    }

    bool isStdOutRedirected()
    {
        // isatty returns a nonzero value if the descriptor is associated with a character device. Otherwise, isatty returns 0.
        return _isatty(_fileno(stdout)) != 0;
    }

    static bool hasTerminalVT100SupportImpl()
    {
        // See https://no-color.org/ for reference.
        const auto noColorEnvVar = getEnvironmentVariable("NO_COLOR");
        if (!noColorEnvVar.empty())
        {
            return false;
        }

        const auto ntdllHandle = GetModuleHandleW(L"ntdll.dll");
        if (ntdllHandle == nullptr)
            return false;

        using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);

#if defined(__MINGW32__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        auto RtlGetVersionFp = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdllHandle, "RtlGetVersion"));
#if defined(__MINGW32__)
#pragma GCC diagnostic pop
#endif
        if (RtlGetVersionFp == nullptr)
            return false;

        RTL_OSVERSIONINFOW info{};
        info.dwOSVersionInfoSize = sizeof(info);

        if (RtlGetVersionFp(&info) != 0)
            return false;

        // VT100 support was first introduced in 10.0.10586
        if (std::tie(info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber) >= std::make_tuple(10U, 0U, 10586U))
            return true;

        return false;
    }

    bool hasTerminalVT100Support()
    {
        static bool hasVT100Support = hasTerminalVT100SupportImpl();
        return hasVT100Support;
    }

    bool enableVT100TerminalMode()
    {
        if (!isStdOutRedirected())
            return false;

        if (!hasTerminalVT100Support())
            return false;

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

        if (hOut == INVALID_HANDLE_VALUE)
            return false;

        DWORD dwMode = 0;

        if (!GetConsoleMode(hOut, &dwMode))
            return false;

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        if (!SetConsoleMode(hOut, dwMode))
            return false;

        return true;
    }

    std::vector<std::string> getCmdLineVector(int, const char**)
    {
        // Discard the input as it will either be ASCII or not available

        // We need to get the Wide command line and convert it to utf8 command line parameters
        const auto cmdline = GetCommandLineW();

        int argc{};
        auto* argw = CommandLineToArgvW(cmdline, &argc);
        if (argw == nullptr)
        {
            std::cerr << "CommandLineToArgvW failed to convert commandline";
            // We will continue but just ignore the command line args.
            argc = 0;
        }

        // This will hold a utf8 string of the command line
        std::vector<std::string> argvStrs;
        argvStrs.resize(argc);

        for (auto i = 0; i < argc; ++i)
        {
            int length = WideCharToMultiByte(CP_UTF8, 0, argw[i], -1, 0, 0, NULL, NULL);

            if (length == 0)
            {
                // Sadly can't print what the argument is that is causing the issue as it needs to be in utf8...
                std::cerr << "Failed to get cmdline argument utf8 length.";
                continue;
            }
            argvStrs[i].resize(length);

            if (WideCharToMultiByte(CP_UTF8, 0, argw[i], -1, argvStrs[i].data(), length, NULL, NULL) == 0)
            {
                std::cerr << "Failed to convert cmdline argument to utf8.";
            }
        }
        LocalFree(reinterpret_cast<HLOCAL>(argw));
        return argvStrs;
    }
}

#endif
