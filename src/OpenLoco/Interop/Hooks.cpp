#include <cassert>
#include <cstdio>
#include <cstring>
#include <system_error>
#ifndef _WIN32
#include <cinttypes>
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "../Audio/Audio.h"
#include "../Console.h"
#include "../Core/FileSystem.hpp"
#include "../Entities/EntityManager.h"
#include "../Environment.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Gui.h"
#include "../Input.h"
#include "../Map/AnimationManager.h"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
#include "../Map/WaveManager.h"
#include "../Paint/Paint.h"
#include "../Platform/Platform.h"
#include "../S5/S5.h"
#include "../Scenario.h"
#include "../Station.h"
#include "../StationManager.h"
#include "../Title.h"
#include "../Tutorial.h"
#include "../Ui.h"
#include "../Ui/ProgressBar.h"
#include "../Ui/WindowManager.h"
#include "../Utility/String.hpp"
#include "../Vehicles/Vehicle.h"
#include "../ViewportManager.h"
#include "../Widget.h"
#include "Interop.hpp"

using namespace OpenLoco;

#define STUB() Console::logVerbose(__FUNCTION__)

#ifdef __i386__
namespace compat = std;
#endif

#ifdef _MSC_VER
#define STDCALL __stdcall
#define CDECL __cdecl
#elif defined(__GNUC__)
#define STDCALL __attribute__((stdcall))
#define CDECL __attribute__((cdecl))
#else
#error Unknown compiler, please define STDCALL and CDECL
#endif

#ifdef __x86_64__
#undef STDCALL
#define STDCALL
#undef CDECL
#define CDECL
#endif

#pragma warning(push)
// MSVC ignores C++17's [[maybe_unused]] attribute on functions, so just disable the warning
#pragma warning(disable : 4505) // unreferenced local function has been removed.

FORCE_ALIGN_ARG_POINTER
static int32_t CDECL audioLoadChannel(int a0, const char* a1, int a2, int a3, int a4)
{
    return Audio::loadChannel((Audio::ChannelId)a0, a1, a2) ? 1 : 0;
}

FORCE_ALIGN_ARG_POINTER
static int32_t CDECL audioPlayChannel(int a0, int a1, int a2, int a3, int a4)
{
    return Audio::playChannel((Audio::ChannelId)a0, a1, a2, a3, a4) ? 1 : 0;
}

FORCE_ALIGN_ARG_POINTER
static void CDECL audioStopChannel(int a0, int a1, int a2, int a3, int a4)
{
    Audio::stopChannel((Audio::ChannelId)a0);
}

FORCE_ALIGN_ARG_POINTER
static void CDECL audioSetChannelVolume(int a0, int a1)
{
    Audio::setChannelVolume((Audio::ChannelId)a0, a1);
}

FORCE_ALIGN_ARG_POINTER
static int32_t CDECL audioIsChannelPlaying(int a0)
{
    return Audio::isChannelPlaying((Audio::ChannelId)a0) ? 1 : 0;
}

#ifdef _NO_LOCO_WIN32_

static void STDCALL fn_404b68(int a0, int a1, int a2, int a3)
{
    STUB();
    return;
}

static int STDCALL getNumDSoundDevices()
{
    STUB();
    return 0;
}
#endif // _NO_LOCO_WIN32_

#pragma pack(push, 1)

struct palette_entry_t
{
    uint8_t b, g, r, a;
};
#pragma pack(pop)
using set_palette_func = void (*)(const palette_entry_t* palette, int32_t index, int32_t count);
static Interop::loco_global<set_palette_func, 0x0052524C> set_palette_callback;

#ifdef _NO_LOCO_WIN32_
FORCE_ALIGN_ARG_POINTER
static void CDECL fn_4054a3(const palette_entry_t* palette, int32_t index, int32_t count)
{
    (*set_palette_callback)(palette, index, count);
}

FORCE_ALIGN_ARG_POINTER
static bool STDCALL fn_40726d()
{
    STUB();
    return true;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_timeGetTime()
{
    return platform::getTime();
}

//typedef bool (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, char*, char*, void*);
FORCE_ALIGN_ARG_POINTER
static long STDCALL fn_DirectSoundEnumerateA(void* pDSEnumCallback, void* pContext)
{
    STUB();
    return 0;
}

static void STDCALL fn_4078be()
{
    STUB();
    return;
}

///region Progress bar

static void CDECL fn_4080bb(char* lpWindowName, uint32_t a1)
{
    Console::log("Create progress bar");
}

static void CDECL fn_408163()
{
    Console::log("Destroy progress bar");
}

static void CDECL fn_40817b(uint16_t arg0)
{
    Console::log("SendMessage(PBM_SETRANGE, %d)", arg0);
    Console::log("SendMessage(PBM_SETSTEP, %d)", 1);
}

static void CDECL fn_4081ad(int32_t wParam)
{
    Console::log("SendMessage(PBM_SETPOS, %d)", wParam);
}

///endregion

struct FileWrapper
{
    FILE* file;
    std::string name;
};

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekSet(FileWrapper* a0, int32_t distance)
{
    Console::logVerbose("seek %d bytes from start", distance);
    fseek(a0->file, distance, SEEK_SET);
    return ftell(a0->file);
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekFromCurrent(FileWrapper* a0, int32_t distance)
{
    Console::logVerbose("seek %d bytes from current", distance);
    fseek(a0->file, distance, SEEK_CUR);
    return ftell(a0->file);
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekFromEnd(FileWrapper* a0, int32_t distance)
{
    Console::logVerbose("seek %d bytes from end", distance);
    fseek(a0->file, distance, SEEK_END);
    return ftell(a0->file);
}

FORCE_ALIGN_ARG_POINTER
static int32_t CDECL fn_FileRead(FileWrapper* a0, char* buffer, int32_t size)
{
    Console::logVerbose("read %d bytes (%d)", size, fileno(a0->file));
    size = fread(buffer, 1, size, a0->file);

    return size;
}

typedef struct FindFileData
{
    uint32_t dwFileAttributes;
    uint32_t ftCreationTime[2];
    uint32_t ftLastAccessTime[2];
    uint32_t ftLastWriteTime[2];
    uint32_t nFileSizeHigh;
    uint32_t nFileSizeLow;
    uint32_t r0;
    uint32_t r1;
    char cFilename[260];
    char cAlternateFileName[14];
} FindFileData;

class Session
{
public:
    std::vector<fs::path> fileList;
};

#define FILE_ATTRIBUTE_DIRECTORY 0x10

FORCE_ALIGN_ARG_POINTER
static Session* CDECL fn_FindFirstFile(char* lpFileName, FindFileData* out)
{
    Console::logVerbose("%s (%s)", __FUNCTION__, lpFileName);

    Session* data = new Session;

    fs::path path = lpFileName;
    path.remove_filename();

    fs::directory_iterator iter(path), end;

    while (iter != end)
    {
        data->fileList.push_back(iter->path());
        ++iter;
    }

    Utility::strcpy_safe(out->cFilename, data->fileList[0].filename().u8string().c_str());
    if (fs::is_directory(data->fileList[0]))
    {
        out->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
        out->dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
    }

    data->fileList.erase(data->fileList.begin());
    return data;
}

static bool CDECL fn_FindNextFile(Session* data, FindFileData* out)
{
    STUB();

    if (data->fileList.size() == 0)
    {
        return false;
    }

    Utility::strcpy_safe(out->cFilename, data->fileList[0].filename().u8string().c_str());
    if (fs::is_directory(data->fileList[0]))
    {
        out->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
        out->dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
    }

    data->fileList.erase(data->fileList.begin());

    return true;
}

static void CDECL fn_FindClose(Session* data)
{
    STUB();

    delete data;
}
#endif // _NO_LOCO_WIN32_

[[maybe_unused]] static void CDECL fnc0(void)
{
    STUB();
}

[[maybe_unused]] static void CDECL fnc1(int i1)
{
    STUB();
}

[[maybe_unused]] static void CDECL fnc2(int i1, int i2)
{
    STUB();
}

[[maybe_unused]] static void STDCALL fn0()
{
    return;
}

[[maybe_unused]] static void STDCALL fn1(int i1)
{
    return;
}

[[maybe_unused]] static void STDCALL fn2(int i1, int i2)
{
    STUB();
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_malloc(uint32_t size)
{
    void* pVoid = malloc(size);
    Console::log("Allocated 0x%X bytes at 0x%" PRIXPTR, size, (uintptr_t)pVoid);
    return (loco_ptr)pVoid;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_realloc(void* block, uint32_t size)
{
    Console::log("Reallocated %" PRIXPTR " to 0x%X bytes", (uintptr_t)block, size);
    return (loco_ptr)realloc(block, size);
}

#ifdef _NO_LOCO_WIN32_
FORCE_ALIGN_ARG_POINTER
static void CDECL fn_free(void* block)
{
    return free(block);
}

enum
{
    DS_OK = 0,
    DSERR_NODRIVER = 0x88780078,
};

static uint32_t STDCALL lib_DirectSoundCreate(void* lpGuid, void* ppDS, void* pUnkOuter)
{
    Console::log("lib_DirectSoundCreate(%lx, %lx, %lx)", (uintptr_t)lpGuid, (uintptr_t)ppDS, (uintptr_t)pUnkOuter);

    return DSERR_NODRIVER;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_CreateRectRgn(int x1, int y1, int x2, int y2)
{
    Console::log("CreateRectRgn(%d, %d, %d, %d)", x1, y1, x2, y2);
    return 0;
}

static uint32_t STDCALL lib_GetUpdateRgn(uintptr_t hWnd, uintptr_t hRgn, bool bErase)
{
    Console::log("GetUpdateRgn(%lx, %lx, %d)", hWnd, hRgn, bErase);
    return 0;
}

static void* STDCALL lib_OpenMutexA(uint32_t dwDesiredAccess, bool bInheritHandle, char* lpName)
{
    Console::log("OpenMutexA(0x%x, %d, %s)", dwDesiredAccess, bInheritHandle, lpName);

    return nullptr;
}

static bool STDCALL lib_DeleteFileA(char* lpFileName)
{
    Console::log("DeleteFileA(%s)", lpFileName);

    return false;
}

FORCE_ALIGN_ARG_POINTER
static bool STDCALL lib_WriteFile(
    FileWrapper* hFile,
    char* buffer,
    size_t nNumberOfBytesToWrite,
    uint32_t* lpNumberOfBytesWritten,
    uintptr_t lpOverlapped)
{
    auto str = std::string(buffer, nNumberOfBytesToWrite);
    size_t i = fwrite(buffer, 1, nNumberOfBytesToWrite, hFile->file);
    *lpNumberOfBytesWritten = i;
    Console::logVerbose("WriteFile(%s)", buffer);

    return true;
}

#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000

#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5

FORCE_ALIGN_ARG_POINTER
static int32_t STDCALL lib_CreateFileA(
    char* lpFileName,
    uint32_t dwDesiredAccess,
    uint32_t dwShareMode,
    uintptr_t lpSecurityAttributes,
    uint32_t dwCreationDisposition,
    uint32_t dwFlagsAndAttributes,
    uintptr_t hTemplateFile)
{
    Console::logVerbose("CreateFile(%s, 0x%x, 0x%x)", lpFileName, dwDesiredAccess, dwCreationDisposition);

    FILE* pFILE = nullptr;
    if (dwDesiredAccess == GENERIC_READ && dwCreationDisposition == OPEN_EXISTING)
    {
        pFILE = fopen(lpFileName, "r");
    }
    else if (dwDesiredAccess == GENERIC_WRITE && dwCreationDisposition == CREATE_ALWAYS)
    {
        pFILE = fopen(lpFileName, "w");
    }
    else
    {
        assert(false);
    }

    if (pFILE == nullptr)
    {
        return -1;
    }

    auto wrapper = new FileWrapper;
    wrapper->file = pFILE;
    wrapper->name = lpFileName;
    return (loco_ptr)wrapper;
}

FORCE_ALIGN_ARG_POINTER
static bool STDCALL lib_SetFileAttributesA(char* lpFileName, uint32_t dwFileAttributes)
{
    // FILE_ATTRIBUTE_NORMAL = 0x80
    assert(dwFileAttributes == 0x80);
    Console::log("SetFileAttributes(%s, %x)", lpFileName, dwFileAttributes);

    std::error_code ec;
    auto path = fs::path(lpFileName);
    auto perms = fs::status(path, ec).permissions();
    if (!ec)
    {
        lib_CreateFileA(lpFileName, dwFileAttributes, 0, 0, 0, 0, 0);
    }
    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write | perms, ec);
    return !ec;
}

static void* STDCALL lib_CreateMutexA(uintptr_t lmMutexAttributes, bool bInitialOwner, char* lpName)
{
    Console::log("CreateMutexA(0x%lx, %d, %s)", lmMutexAttributes, bInitialOwner, lpName);

    return nullptr;
}

FORCE_ALIGN_ARG_POINTER
static bool STDCALL lib_CloseHandle(void* hObject)
{
    auto file = (FileWrapper*)hObject;

    return fclose(file->file) == 0;
}

FORCE_ALIGN_ARG_POINTER
static void STDCALL lib_PostQuitMessage(int32_t exitCode)
{
    Console::log("lib_PostQuitMessage(%d)", exitCode);
    exit(exitCode);
}
#endif // _NO_LOCO_WIN32_
#pragma warning(pop)

static void registerMemoryHooks()
{
    using namespace OpenLoco::Interop;

    // Hook Locomotion's alloc / free routines so that we don't
    // allocate a block in one module and freeing it in another.
    hookFunction(0x4d1401, CallingConvention::cdecl, 1, (void (*)()) & fn_malloc);
    hookFunction(0x4D1B28, CallingConvention::cdecl, 2, (void (*)()) & fn_realloc);
    hookFunction(0x4D1355, CallingConvention::cdecl, 1, (void (*)()) & fn_free);
}

#ifdef _NO_LOCO_WIN32_
static void registerNoWin32Hooks()
{
    using namespace OpenLoco::Interop;

    hookFunction(0x404b68, CallingConvention::stdcall, 4, (void (*)()) & fn_404b68);
    hookFunction(0x404e8c, CallingConvention::stdcall, 0, (void (*)()) & getNumDSoundDevices);
    hookFunction(0x4064fa, CallingConvention::stdcall, 0, (void (*)()) & fn0);
    hookFunction(0x40726d, CallingConvention::stdcall, 0, (void (*)()) & fn_40726d);
    hookFunction(0x4054a3, CallingConvention::cdecl, 3, (void (*)()) & fn_4054a3);
    hookFunction(0x4072ec, CallingConvention::stdcall, 0, (void (*)()) & fn0);
    hookFunction(0x4078be, CallingConvention::stdcall, 0, (void (*)()) & fn_4078be);
    hookFunction(0x4080bb, CallingConvention::cdecl, 2, (void (*)()) & fn_4080bb);
    hookFunction(0x408163, CallingConvention::cdecl, 0, (void (*)()) & fn_408163);
    hookFunction(0x40817b, CallingConvention::cdecl, 1, (void (*)()) & fn_40817b);
    hookFunction(0x4081ad, CallingConvention::cdecl, 1, (void (*)()) & fn_4081ad);
    hookFunction(0x4081c5, CallingConvention::cdecl, 2, (void (*)()) & fn_FileSeekSet);
    hookFunction(0x4081d8, CallingConvention::cdecl, 2, (void (*)()) & fn_FileSeekFromCurrent);
    hookFunction(0x4081eb, CallingConvention::cdecl, 2, (void (*)()) & fn_FileSeekFromEnd);
    hookFunction(0x4081fe, CallingConvention::cdecl, 3, (void (*)()) & fn_FileRead);
    hookFunction(0x40830e, CallingConvention::cdecl, 2, (void (*)()) & fn_FindFirstFile);
    hookFunction(0x40831d, CallingConvention::cdecl, 2, (void (*)()) & fn_FindNextFile);
    hookFunction(0x40832c, CallingConvention::cdecl, 1, (void (*)()) & fn_FindClose);
    hookFunction(0x4d0fac, CallingConvention::stdcall, 2, (void (*)()) & fn_DirectSoundEnumerateA);

    // fill DLL hooks for ease of debugging
    for (uint32_t address = 0x4d7000; address <= 0x4d72d8; address += 4)
    {
        hookLibrary(address, [address]() {
            Console::log("Missing hook: 0x%x", address);
        });
    }

    // dsound.dll
    hookLibrary(0x4d7024, CallingConvention::stdcall, 3, (void (*)()) & lib_DirectSoundCreate);

    // gdi32.dll
    hookLibrary(0x4d7078, CallingConvention::stdcall, 4, (void (*)()) & lib_CreateRectRgn);

    // kernel32.dll
    hookLibrary(0x4d70e0, CallingConvention::stdcall, 3, (void (*)()) & lib_CreateMutexA);
    hookLibrary(0x4d70e4, CallingConvention::stdcall, 3, (void (*)()) & lib_OpenMutexA);
    hookLibrary(0x4d70f0, CallingConvention::stdcall, 5, (void (*)()) & lib_WriteFile);
    hookLibrary(0x4d70f4, CallingConvention::stdcall, 1, (void (*)()) & lib_DeleteFileA);
    hookLibrary(0x4d70f8, CallingConvention::stdcall, 2, (void (*)()) & lib_SetFileAttributesA);
    hookLibrary(0x4d70fC, CallingConvention::stdcall, 7, (void (*)()) & lib_CreateFileA);

    // user32.dll
    hookLibrary(0x4d71e8, CallingConvention::stdcall, 1, (void (*)()) & lib_PostQuitMessage);
    hookLibrary(0x4d714c, CallingConvention::stdcall, 1, (void (*)()) & lib_CloseHandle);
    hookLibrary(0x4d7248, CallingConvention::stdcall, 3, (void (*)()) & lib_GetUpdateRgn);
    hookLibrary(0x4d72b0, CallingConvention::stdcall, 0, (void (*)()) & lib_timeGetTime);
}
#endif // _NO_LOCO_WIN32_

void OpenLoco::Interop::loadSections()
{
#ifndef _WIN32
    int32_t err = mprotect((void*)0x401000, 0x4d7000 - 0x401000, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (err != 0)
    {
        perror("mprotect");
    }

    err = mprotect((void*)0x4d7000, 0x1162000 - 0x4d7000, PROT_READ | PROT_WRITE);
    if (err != 0)
    {
        perror("mprotect");
    }
#endif
}

static void registerAudioHooks()
{
    using namespace OpenLoco::Interop;

    hookFunction(0x0040194E, CallingConvention::cdecl, 5, (void (*)()) & audioLoadChannel);
    hookFunction(0x00401999, CallingConvention::cdecl, 5, (void (*)()) & audioPlayChannel);
    hookFunction(0x00401A05, CallingConvention::cdecl, 5, (void (*)()) & audioStopChannel);
    hookFunction(0x00401AD3, CallingConvention::cdecl, 2, (void (*)()) & audioSetChannelVolume);
    hookFunction(0x00401B10, CallingConvention::cdecl, 1, (void (*)()) & audioIsChannelPlaying);

    writeRet(0x0048AB36);
    writeRet(0x00404B40);
    registerHook(
        0x0048A18C,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Audio::updateSounds();
            return 0;
        });
    registerHook(
        0x00489C6A,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Audio::stopVehicleNoise();
            return 0;
        });
    registerHook(
        0x0048A4BF,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Audio::playSound(X86Pointer<Vehicles::Vehicle2or6>(regs.esi));
            return 0;
        });
    registerHook(
        0x00489CB5,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Audio::playSound((Audio::SoundId)regs.eax, { regs.cx, regs.dx, regs.bp }, regs.ebx);
            return 0;
        });
    registerHook(
        0x00489F1B,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Audio::playSound((Audio::SoundId)regs.eax, { regs.cx, regs.dx, regs.bp }, regs.edi, regs.ebx);
            return 0;
        });
}

void OpenLoco::Interop::registerHooks()
{
    using namespace OpenLoco::Ui::Windows;

    registerMemoryHooks();

#ifdef _NO_LOCO_WIN32_
    registerNoWin32Hooks();
#endif // _NO_LOCO_WIN32_

    registerAudioHooks();

    registerHook(
        0x00431695,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;

            OpenLoco::sub_431695(0);
            regs = backup;
            return 0;
        });

    registerHook(
        0x004416B5,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            using namespace OpenLoco::Environment;

            auto buffer = (char*)0x009D0D72;
            auto path = getPath((path_id)regs.ebx);

            // TODO: use Utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(buffer, path.make_preferred().u8string().c_str());

            regs.ebx = X86Pointer(buffer);
            return 0;
        });

    // Replace Ui::update() with our own
    registerHook(
        0x004524C1,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Ui::update();
            return 0;
        });

    registerHook(
        0x00407BA3,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            auto cursor = (Ui::CursorId)regs.edx;
            Ui::setCursor(cursor);
            return 0;
        });

    registerHook(
        0x00446F6B,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            auto result = Ui::Windows::PromptOkCancel::open(regs.eax);
            regs.eax = result ? 1 : 0;
            return 0;
        });

    registerHook(
        0x00407231,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            OpenLoco::Input::sub_407231();
            return 0;
        });

    registerHook(
        0x00451025,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            auto pos = Gfx::drawString(*X86Pointer<Gfx::Context>(regs.edi), regs.cx, regs.dx, regs.al, X86Pointer<uint8_t>(regs.esi));
            regs = backup;
            regs.cx = pos.x;
            regs.dx = pos.y;

            return 0;
        });

    // Until handling of State::viewportLeft has been implemented in mouse_input...
    registerHook(
        0x00490F6C,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Ui::Windows::StationList::open(regs.ax);
            return 0;
        });

    registerHook(
        0x004958C6,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            char* buffer = X86Pointer<char>(regs.edi);
            void* args = X86Pointer(regs.ecx);
            buffer = StringManager::formatString(buffer, regs.eax, args);
            regs = backup;
            regs.edi = X86Pointer(buffer);
            return 0;
        });

    registerHook(
        0x00438A6C,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Gui::init();
            return 0;
        });

    /* This can be removed after implementing signal place and remove game commands.
     * It fixes an original bug with those two game commands.
     */
    registerHook(
        0x004A2AD7,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            addr<0x001135F88, uint16_t>() = 0;
            regs.esi = 0x004A2AF0;
            regs.edi = 0x004A2CE7;
            call(0x004A2E46, regs);

            // Set ebp register to a nice large non-negative number.
            // This fixes exorbitant prices for signals on Linux and macOS.
            // Value must be greater than the cost for placing a signal.
            regs.ebp = 0xC0FFEE;
            return 0;
        });

    registerHook(
        0x004CA4DF,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::Window* window = X86Pointer<Ui::Window>(regs.esi);
            auto context = X86Pointer<Gfx::Context>(regs.edi);
            window->draw(context);
            regs = backup;
            return 0;
        });

    registerHook(
        0x004CF63B,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Gfx::render();
            regs = backup;
            return 0;
        });

    Ui::ProgressBar::registerHooks();
    Map::TileManager::registerHooks();
    Map::AnimationManager::registerHooks();
    Ui::Windows::PromptBrowse::registerHooks();
    Ui::Windows::TextInput::registerHooks();
    Ui::Windows::ToolTip::registerHooks();
    Ui::Windows::Vehicle::registerHooks();
    Ui::Windows::BuildVehicle::registerHooks();
    Ui::Windows::Terraform::registerHooks();
    Ui::Windows::Error::registerHooks();
    Ui::Windows::Construction::registerHooks();
    Ui::WindowManager::registerHooks();
    Ui::ViewportManager::registerHooks();
    GameCommands::registerHooks();
    Scenario::registerHooks();
    StationManager::registerHooks();
    S5::registerHooks();
    Title::registerHooks();
    OpenLoco::Tutorial::registerHooks();
    Paint::registerHooks();

    // Part of 0x004691FA
    registerHook(
        0x0046956E,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Map::Pos2 pos(regs.ax, regs.cx);
            Map::SurfaceElement* surface = X86Pointer<Map::SurfaceElement>(regs.esi);

            WaveManager::createWave(*surface, pos);

            regs = backup;
            return 0;
        });

    registerHook(
        0x004AB655,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Vehicles::VehicleBase* v = X86Pointer<Vehicles::VehicleBase>(regs.esi);
            v->asVehicleBody()->secondaryAnimationUpdate();

            return 0;
        });

    registerHook(
        0x004392BD,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            Gui::resize();
            return 0;
        });

    registerHook(
        0x004C6456,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::Window* window = X86Pointer<Ui::Window>(regs.esi);
            window->viewportsUpdatePosition();
            regs = backup;
            return 0;
        });

    registerHook(
        0x004C9513,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::Window* window = X86Pointer<Ui::Window>(regs.esi);
            int16_t x = regs.ax;
            int16_t y = regs.bx;

            auto widgetIndex = window->findWidgetAt(x, y);

            regs = backup;
            regs.edx = widgetIndex;
            if (widgetIndex == -1)
            {
                regs.edi = X86Pointer(&window->widgets[0]);
            }
            else
            {
                regs.edi = X86Pointer(&window->widgets[widgetIndex]);
            }

            return 0;
        });

    registerHook(
        0x004CA115,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::Window* window = X86Pointer<Ui::Window>(regs.esi);
            window->updateScrollWidgets();
            regs = backup;

            return 0;
        });

    registerHook(
        0x004CA17F,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::Window* window = X86Pointer<Ui::Window>(regs.esi);
            window->initScrollWidgets();
            regs = backup;

            return 0;
        });

    registerHook(
        0x004C57C0,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;

            initialiseViewports();
            regs = backup;
            return 0;
        });

    registerHook(
        0x004C5DD5,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;

            Gfx::redrawScreenRect(regs.ax, regs.bx, regs.dx, regs.bp);

            regs = backup;
            return 0;
        });

    // Remove check for is road in use when removing roads. It is
    // quite annoying when it's sometimes only the player's own
    // vehicles that are using it.
    writeNop(0x004776DD, 6);

    registerHook(
        0x0047024A,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;

            auto* entity = reinterpret_cast<EntityBase*>(regs.esi);
            EntityManager::freeEntity(entity);

            regs = backup;
            return 0;
        });
}
