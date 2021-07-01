#include "Crash.h"
#if defined(USE_BREAKPAD)
#include "../OpenLoco.h"
#include "../Utility/String.hpp"
#include "Platform.h"

#ifdef _WIN32 // _WIN32

#include <ShlObj.h>
#include <client/windows/handler/exception_handler.h>

static bool onCrash(
    const wchar_t* dumpPath, const wchar_t* miniDumpId, void* context, EXCEPTION_POINTERS* exinfo,
    MDRawAssertionInfo* assertion, bool succeeded)
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
                                             L"the dump and most recently saved game there.\n\n\nVersion: %s\n\n";
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

static std::wstring getDumpDirectory()
{
    auto user_dir = OpenLoco::platform::getUserDirectory();
    auto result = OpenLoco::Utility::toUtf16(user_dir.string());
    return result;
}
#endif // _WIN32

#ifdef __linux__ // __linux__

#include "client/linux/handler/exception_handler.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"

#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>

#include <SDL2/SDL.h>

static int my_copy(const char* to, const char* from)
{
    const int fromFd = sys_open(from, O_RDONLY, 0600);
    if (fromFd < 0)
    {
        return -1;
    }
    const int toFd = sys_open(to, O_WRONLY | O_CREAT, 0600);
    if (toFd < 0)
    {
        sys_close(fromFd);
        return -1;
    }
    char buf[4096]; // size less than temp stack size, otherwise overflow
    ssize_t n_read;
    while (true)
    {
        n_read = sys_read(fromFd, buf, sizeof(buf));
        if (n_read <= 0)
        {
            break;
        }

        char* out_ptr = buf;
        ssize_t n_write;
        while (true)
        {
            n_write = sys_write(toFd, out_ptr, n_read);
            if (n_write >= 0)
            {
                n_read -= n_write;
                out_ptr += n_write;
            }
            else
            {
                sys_close(toFd);
                sys_close(fromFd);
                return -1;
            }

            if (n_read <= 0)
            {
                break;
            }
        }
    }

    if (n_read == 0)
    {
        sys_close(toFd);
        sys_close(fromFd);
        return 0;
    }
    else
    {
        sys_close(toFd);
        sys_close(fromFd);
        return -1;
    }
}

static bool onCrash(
    const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
    constexpr const char* dumpFailedMessage = "Failed to create the dump. Please file an issue with OpenLoco on GitHub and "
                                              "provide latest save, and provide "
                                              "information about what you did before the crash occurred.\n";

    char dumpPath[PATH_MAX];
    char dumpPathNew[PATH_MAX];
    char message[PATH_MAX];

    if (succeeded)
    {
        my_strlcpy(dumpPath, descriptor.path(), std::size(dumpPath));
        if (dumpPath[std::size(dumpPath) - 1] != '\0')
        {
            dumpPath[std::size(dumpPath) - 1] = '\0';
        }
        size_t dumpPathLen = my_strlen(dumpPath);
        size_t versionLen = my_strlen(OpenLoco::version);

        if (dumpPathLen + versionLen < std::size(dumpPathNew))
        {
            // insert version info
            size_t dot_pos = dumpPathLen;
            size_t cur_pos = 0;
            while (dumpPath[--dot_pos] != '.')
                ;
            for (; cur_pos < dot_pos; cur_pos++)
            {
                dumpPathNew[cur_pos] = dumpPath[cur_pos];
            }
            dumpPathNew[cur_pos++] = '(';
            for (size_t version_pos = 0; version_pos < versionLen; version_pos++, cur_pos++)
            {
                dumpPathNew[cur_pos] = OpenLoco::version[version_pos];
            }
            dumpPathNew[cur_pos++] = ')';
            for (size_t ext_pos = 0; dot_pos + ext_pos < dumpPathLen; dot_pos++, cur_pos++)
            {
                dumpPathNew[cur_pos] = dumpPath[dot_pos + ext_pos];
            }
            dumpPathNew[cur_pos] = '\0';

            // try to rename dump file
            // breakpad seems not to implement syscall `rename` or `renameat`, so copy the dump and remove the original one
            // use custom copy function as breakpad do not implement `sendfile`
            int copy_success = my_copy(dumpPathNew, dumpPath);
            if (copy_success == 0)
            {
                // remove original dump
                sys_unlink(dumpPath);
                // overwrite new filename
                my_strlcpy(dumpPath, dumpPathNew, std::size(dumpPath));
            }
        }

        // generate nessage
        my_strlcpy(message, "A crash has occurred and a dump was created at\n", std::size(message));
        my_strlcat(message, dumpPath, std::size(message));
        my_strlcat(message, "\n\nPlease file an issue with OpenLoco on GitHub and provide the dump and most recently saved game there.\n\n\nVersion: ", std::size(message));
        my_strlcat(message, OpenLoco::version, std::size(message));
        my_strlcat(message, "\n\n", std::size(message));
        if (message[std::size(message) - 1] != '\0')
        {
            message[std::size(message) - 1] = '\0';
        }
    }

    // writes to stdout
    if (!succeeded)
    {
        sys_write(STDERR_FILENO, dumpFailedMessage, my_strlen(dumpFailedMessage));
    }
    else
    {
        sys_write(STDERR_FILENO, message, my_strlen(message));
    }

    // danger zone: create message box with SDL;
    // from now code will access libc.so
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Can not initialize SDL: %s\n", SDL_GetError());
    }

    if (!succeeded)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "OpenLoco",
            dumpFailedMessage,
            nullptr);
    }
    else
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_WARNING,
            "OpenLoco",
            message,
            nullptr);
    }

    return succeeded;
}

static std::string getDumpDirectory()
{
    return OpenLoco::platform::getUserDirectory().string();
}

#endif // __linux__

#endif // USE_BREAKPAD

// Using non-null pipe name here lets breakpad try setting OOP crash handling
constexpr const wchar_t* PipeName = L"openloco-bpad";

CExceptionHandler crashInit()
{
#if defined(USE_BREAKPAD)
#ifdef _WIN32
    // Path must exist and be RW!
    auto exHandler = new google_breakpad::ExceptionHandler(
        getDumpDirectory(), 0, onCrash, 0, google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpWithDataSegs, PipeName, 0);
#endif //

#ifdef __linux__
    // Path must exist and be RW!
    auto exHandler = new google_breakpad::ExceptionHandler(
        google_breakpad::MinidumpDescriptor{ getDumpDirectory() }, nullptr, onCrash, nullptr, true, -1);
#endif

    return exHandler;

#else  // USE_BREAKPAD
    return nullptr;
#endif // USE_BREAKPAD
}

void crashClose(CExceptionHandler exHandler)
{
#if defined(USE_BREAKPAD)
    delete exHandler;
#endif // USE_BREAKPAD
}
