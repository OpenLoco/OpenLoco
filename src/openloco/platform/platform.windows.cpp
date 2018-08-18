#ifdef _WIN32

#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
// We can't use lean and mean if we want timeGetTime
// #define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

#include "../ui.h"
#include "../utility/string.hpp"
#include "platform.h"

namespace openloco::platform
{
    uint32_t get_time()
    {
        return timeGetTime();
    }

    fs::path get_user_directory()
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

    static std::wstring SHGetPathFromIDListLongPath(LPCITEMIDLIST pidl)
    {
        std::wstring pszPath(MAX_PATH, 0);
        while (!SHGetPathFromIDListEx(pidl, &pszPath[0], (DWORD)pszPath.size(), 0))
        {
            if (pszPath.size() >= SHRT_MAX)
            {
                // Clearly not succeeding at all, bail
                return std::wstring();
            }
            pszPath.resize(pszPath.size() * 2);
        }
        return pszPath;
    }

    std::string prompt_directory(const std::string& title)
    {
        std::string result;

        // Initialize COM and get a pointer to the shell memory allocator
        LPMALLOC lpMalloc;
        if (SUCCEEDED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)) && SUCCEEDED(SHGetMalloc(&lpMalloc)))
        {
            auto titleW = utility::to_utf16(title);
            BROWSEINFOW bi = { 0 };
            bi.lpszTitle = titleW.c_str();
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl != nullptr)
            {
                result = utility::to_utf8(SHGetPathFromIDListLongPath(pidl));
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
        ShowWindow((HWND)ui::hwnd(), SW_RESTORE);

        return result;
    }

    static std::string WIN32_GetModuleFileNameW(HMODULE hModule)
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
        return utility::to_utf8(wExePath.get());
    }

    fs::path GetCurrentExecutablePath()
    {
        return WIN32_GetModuleFileNameW(nullptr);
    }
}

#endif
