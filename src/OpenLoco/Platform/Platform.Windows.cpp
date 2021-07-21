#ifdef _WIN32

#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
// We can't use lean and mean if we want timeGetTime
// #define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

#include "../Ui.h"
#include "../Utility/String.hpp"
#include "Platform.h"

namespace OpenLoco::platform
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

    fs::path promptDirectory(const std::string& title)
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
        ShowWindow((HWND)Ui::hwnd(), SW_RESTORE);

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

    fs::path GetCurrentExecutablePath()
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
                drives.push_back(drive);
            }
        }
        return drives;
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
}

#endif
