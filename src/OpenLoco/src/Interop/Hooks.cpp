#include "Hooks.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <system_error>
#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "Audio/Audio.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Gui.h"
#include "Input.h"
#include "Localisation/Formatting.h"
#include "Logging.h"
#include "Map/AnimationManager.h"
#include "Map/Tile.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/WaveManager.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Paint/Paint.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "Title.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"
#include "World/Station.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Utility/String.hpp>

using namespace OpenLoco;
using namespace OpenLoco::Diagnostics;

#define STUB() Logging::verbose("{}", __FUNCTION__)

#ifdef _MSC_VER
#define STDCALL __stdcall
#define CDECL __cdecl
#elif defined(__GNUC__)
#define STDCALL __attribute__((stdcall))
#define CDECL __attribute__((cdecl))
#else
#error Unknown compiler, please define STDCALL and CDECL
#endif

#pragma warning(push)
// MSVC ignores C++17's [[maybe_unused]] attribute on functions, so just disable the warning
#pragma warning(disable : 4505) // unreferenced local function has been removed.

#ifdef _NO_LOCO_WIN32_
FORCE_ALIGN_ARG_POINTER
static void STDCALL fn_40447f()
{
    STUB();
    return;
}

FORCE_ALIGN_ARG_POINTER
static void STDCALL fn_404b68(int, int, int, int)
{
    STUB();
    return;
}

FORCE_ALIGN_ARG_POINTER
static int STDCALL getNumDSoundDevices()
{
    STUB();
    return 0;
}
#endif // _NO_LOCO_WIN32_

#pragma pack(push, 1)

struct PaletteEntry
{
    uint8_t b, g, r, a;
};
#pragma pack(pop)
using SetPaletteFunc = void (*)(const PaletteEntry* palette, int32_t index, int32_t count);
static Interop::loco_global<SetPaletteFunc, 0x0052524C> _setPaletteCallback;

#ifdef _NO_LOCO_WIN32_
FORCE_ALIGN_ARG_POINTER
static void CDECL fn_4054a3(const PaletteEntry* palette, int32_t index, int32_t count)
{
    if (_setPaletteCallback != nullptr)
    {
        (*_setPaletteCallback)(palette, index, count);
    }
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_timeGetTime()
{
    return Platform::getTime();
}

// typedef bool (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, char*, char*, void*);
FORCE_ALIGN_ARG_POINTER
static long STDCALL fn_DirectSoundEnumerateA(void*, void*)
{
    STUB();
    return 0;
}

FORCE_ALIGN_ARG_POINTER
static void STDCALL fn_4078be()
{
    STUB();
    return;
}

FORCE_ALIGN_ARG_POINTER
static void STDCALL fn_4078fe()
{
    STUB();
    return;
}

FORCE_ALIGN_ARG_POINTER
static void STDCALL fn_407b26()
{
    STUB();
    return;
}

/// region Progress bar

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_4080bb(char*, uint32_t)
{
    Logging::info("Create progress bar");
}

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_408163()
{
    Logging::info("Destroy progress bar");
}

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_40817b(uint16_t arg0)
{
    Logging::info("SendMessage(PBM_SETRANGE, {})", arg0);
    Logging::info("SendMessage(PBM_SETSTEP, {})", 1);
}

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_4081ad(int32_t wParam)
{
    Logging::info("SendMessage(PBM_SETPOS, {})", wParam);
}

/// endregion

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekSet(FILE* a0, int32_t distance)
{
    Logging::info("seek {} bytes from start", distance);
    fseek(a0, distance, SEEK_SET);
    return ftell(a0);
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekFromCurrent(FILE* a0, int32_t distance)
{
    Logging::verbose("seek {} bytes from current", distance);
    fseek(a0, distance, SEEK_CUR);
    return ftell(a0);
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekFromEnd(FILE* a0, int32_t distance)
{
    Logging::verbose("seek {} bytes from end", distance);
    fseek(a0, distance, SEEK_END);
    return ftell(a0);
}

FORCE_ALIGN_ARG_POINTER
static int32_t CDECL fn_FileRead(FILE* a0, char* buffer, int32_t size)
{
    Logging::verbose("read {} bytes ({})", size, fileno(a0));
    size = fread(buffer, 1, size, a0);

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
    Logging::verbose("{} ({})", __FUNCTION__, lpFileName);

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

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FindNextFile(Session* data, FindFileData* out)
{
    STUB();

    if (data->fileList.size() == 0)
    {
        return 0;
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

    return 1;
}

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_FindClose(Session* data)
{
    STUB();

    delete data;
}
#endif // _NO_LOCO_WIN32_

FORCE_ALIGN_ARG_POINTER
[[maybe_unused]] static void CDECL fnc0(void)
{
    STUB();
}

FORCE_ALIGN_ARG_POINTER
[[maybe_unused]] static void CDECL fnc1(int)
{
    STUB();
}

FORCE_ALIGN_ARG_POINTER
[[maybe_unused]] static void CDECL fnc2(int, int)
{
    STUB();
}

FORCE_ALIGN_ARG_POINTER
[[maybe_unused]] static void STDCALL fn0()
{
    return;
}

FORCE_ALIGN_ARG_POINTER
[[maybe_unused]] static void STDCALL fn1(int)
{
    return;
}

FORCE_ALIGN_ARG_POINTER
[[maybe_unused]] static void STDCALL fn2(int, int)
{
    STUB();
}

FORCE_ALIGN_ARG_POINTER
static void* CDECL fn_malloc(uint32_t size)
{
    return malloc(size);
}

FORCE_ALIGN_ARG_POINTER
static void* CDECL fn_realloc(void* block, uint32_t size)
{
    return realloc(block, size);
}

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_free(void* block)
{
    return free(block);
}

#ifdef _NO_LOCO_WIN32_
FORCE_ALIGN_ARG_POINTER
static void STDCALL fn_dump(uint32_t address)
{
    Logging::info("Missing hook: 0x{:x}", address);
}

enum
{
    DS_OK = 0,
    DSERR_NODRIVER = 0x88780078,
};

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_DirectSoundCreate(void* lpGuid, void* ppDS, void* pUnkOuter)
{
    Logging::info("lib_DirectSoundCreate({:x}, {:x}, {:x})", (uintptr_t)lpGuid, (uintptr_t)ppDS, (uintptr_t)pUnkOuter);

    return DSERR_NODRIVER;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_CreateRectRgn(int x1, int y1, int x2, int y2)
{
    Logging::info("CreateRectRgn({}, {}, {}, {})", x1, y1, x2, y2);
    return 0;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_GetUpdateRgn(uintptr_t hWnd, uintptr_t hRgn, bool bErase)
{
    Logging::info("GetUpdateRgn({:x}, {:x}, {})", hWnd, hRgn, bErase);
    return 0;
}

FORCE_ALIGN_ARG_POINTER
static void* STDCALL lib_OpenMutexA(uint32_t dwDesiredAccess, bool bInheritHandle, char* lpName)
{
    Logging::info("OpenMutexA(0x{:x}, {}, {})", dwDesiredAccess, bInheritHandle, lpName);

    return nullptr;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_DeleteFileA(char* lpFileName)
{
    Logging::info("DeleteFileA({})", lpFileName);

    return 0;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_WriteFile(
    FILE* hFile,
    char* buffer,
    size_t nNumberOfBytesToWrite,
    uint32_t* lpNumberOfBytesWritten,
    uintptr_t)
{
    *lpNumberOfBytesWritten = fwrite(buffer, 1, nNumberOfBytesToWrite, hFile);
    Logging::verbose("WriteFile({})", fmt::ptr(buffer));

    return 1;
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
    uint32_t,
    uintptr_t,
    uint32_t dwCreationDisposition,
    uint32_t,
    uintptr_t)
{
    Logging::verbose("CreateFile({}, 0x{:x}, 0x{:x})", lpFileName, dwDesiredAccess, dwCreationDisposition);

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

    return (int32_t)pFILE;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_SetFileAttributesA(char* lpFileName, uint32_t dwFileAttributes)
{
    // FILE_ATTRIBUTE_NORMAL = 0x80
    assert(dwFileAttributes == 0x80);
    Logging::info("SetFileAttributes({}, {:x})", lpFileName, dwFileAttributes);

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

FORCE_ALIGN_ARG_POINTER
static void* STDCALL lib_CreateMutexA(uintptr_t lmMutexAttributes, bool bInitialOwner, char* lpName)
{
    Logging::info("CreateMutexA(0x{:x}, {}, {})", lmMutexAttributes, bInitialOwner, lpName);

    return nullptr;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_CloseHandle(void* hObject)
{
    auto file = (FILE*)hObject;

    return fclose(file) == 0;
}

FORCE_ALIGN_ARG_POINTER
static void STDCALL lib_PostQuitMessage(int32_t exitCode)
{
    Logging::info("lib_PostQuitMessage({})", exitCode);
    exit(exitCode);
}
#endif // _NO_LOCO_WIN32_
#pragma warning(pop)

static void registerMemoryHooks()
{
    using namespace OpenLoco::Interop;

    // Hook Locomotion's alloc / free routines so that we don't
    // allocate a block in one module and freeing it in another.
    writeJmp(0x4D1401, (void*)&fn_malloc);
    writeJmp(0x406BF7, (void*)&fn_malloc);
    writeJmp(0x4D1B28, (void*)&fn_realloc);
    writeJmp(0x406C02, (void*)&fn_realloc);
    writeJmp(0x4D1355, (void*)&fn_free);
    writeJmp(0x406C12, (void*)&fn_free);
}

#ifdef _NO_LOCO_WIN32_
static void registerNoWin32Hooks()
{
    using namespace OpenLoco::Interop;

    writeJmp(0x40447f, (void*)&fn_40447f);
    writeJmp(0x404b68, (void*)&fn_404b68);
    writeJmp(0x404e8c, (void*)&getNumDSoundDevices);
    writeJmp(0x4064fa, (void*)&fn0);
    writeJmp(0x4054a3, (void*)&fn_4054a3);
    writeJmp(0x4072ec, (void*)&fn0);
    writeJmp(0x4078be, (void*)&fn_4078be);
    writeJmp(0x4078fe, (void*)&fn_4078fe);
    writeJmp(0x407b26, (void*)&fn_407b26);
    writeJmp(0x4080bb, (void*)&fn_4080bb);
    writeJmp(0x408163, (void*)&fn_408163);
    writeJmp(0x40817b, (void*)&fn_40817b);
    writeJmp(0x4081ad, (void*)&fn_4081ad);
    writeJmp(0x4081c5, (void*)&fn_FileSeekSet);
    writeJmp(0x4081d8, (void*)&fn_FileSeekFromCurrent);
    writeJmp(0x4081eb, (void*)&fn_FileSeekFromEnd);
    writeJmp(0x4081fe, (void*)&fn_FileRead);
    writeJmp(0x40830e, (void*)&fn_FindFirstFile);
    writeJmp(0x40831d, (void*)&fn_FindNextFile);
    writeJmp(0x40832c, (void*)&fn_FindClose);
    writeJmp(0x4d0fac, (void*)&fn_DirectSoundEnumerateA);

    // fill DLL hooks for ease of debugging
    for (int i = 0x4d7000; i <= 0x4d72d8; i += 4)
    {
        hookDump(i, (void*)&fn_dump);
    }

    // dsound.dll
    hookLib(0x4d7024, (void*)&lib_DirectSoundCreate);

    // gdi32.dll
    hookLib(0x4d7078, (void*)&lib_CreateRectRgn);

    // kernel32.dll
    hookLib(0x4d70e0, (void*)&lib_CreateMutexA);
    hookLib(0x4d70e4, (void*)&lib_OpenMutexA);
    hookLib(0x4d70f0, (void*)&lib_WriteFile);
    hookLib(0x4d70f4, (void*)&lib_DeleteFileA);
    hookLib(0x4d70f8, (void*)&lib_SetFileAttributesA);
    hookLib(0x4d70fC, (void*)&lib_CreateFileA);

    // user32.dll
    hookLib(0x4d71e8, (void*)&lib_PostQuitMessage);
    hookLib(0x4d714c, (void*)&lib_CloseHandle);
    hookLib(0x4d7248, (void*)&lib_GetUpdateRgn);
    hookLib(0x4d72b0, (void*)&lib_timeGetTime);
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

    writeRet(0x0048AB36);
    writeRet(0x00404B40);
    registerHook(
        0x0048A18C,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Audio::updateSounds();
            regs = backup;
            return 0;
        });
    registerHook(
        0x00489C6A,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Audio::stopVehicleNoise();
            regs = backup;
            return 0;
        });
    registerHook(
        0x0048A4BF,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Audio::playSound(X86Pointer<Vehicles::Vehicle2or6>(regs.esi));
            regs = backup;
            return 0;
        });
    registerHook(
        0x00489CB5,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Audio::playSound((Audio::SoundId)regs.eax, { regs.cx, regs.dx, regs.bp }, regs.ebx);
            regs = backup;
            return 0;
        });
    registerHook(
        0x00489F1B,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Audio::playSound((Audio::SoundId)regs.eax, { regs.cx, regs.dx, regs.bp }, regs.edi, regs.ebx);
            regs = backup;
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
            registers backup = regs;
            using namespace OpenLoco::Environment;

            auto buffer = (char*)0x009D0D72;
            auto path = getPath((PathId)regs.ebx);

            // TODO: use Utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(buffer, path.make_preferred().u8string().c_str());
            regs = backup;
            regs.ebx = X86Pointer(buffer);
            return 0;
        });

    // Replace Ui::update() with our own
    registerHook(
        0x004524C1,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::update();
            regs = backup;
            return 0;
        });

    registerHook(
        0x00407BA3,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            auto cursor = (Ui::CursorId)regs.edx;
            Ui::setCursor(cursor);
            regs = backup;
            return 0;
        });

    registerHook(
        0x00407231,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            OpenLoco::Input::sub_407231();
            regs = backup;
            return 0;
        });

    registerHook(
        0x00451025,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;

            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            const auto& rt = *X86Pointer<Gfx::RenderTarget>(regs.edi);
            drawingCtx.pushRenderTarget(rt);

            auto tr = Gfx::TextRenderer(drawingCtx);
            auto point = Ui::Point(regs.cx, regs.dx);
            tr.drawString(point, static_cast<Colour>(regs.al), X86Pointer<const char>(regs.esi));

            drawingCtx.popRenderTarget();

            regs = backup;

            return 0;
        });

    // Until handling of State::viewportLeft has been implemented in mouse_input...
    registerHook(
        0x00490F6C,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Ui::Windows::StationList::open(CompanyId(regs.ax));
            regs = backup;
            return 0;
        });

    registerHook(
        0x004958C6,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            char* buffer = X86Pointer<char>(regs.edi);
            void* args = X86Pointer(regs.ecx);
            // Welp, not properly bounded but short term.
            auto fmtArgs = FormatArguments{ static_cast<std::byte*>(args), 40 };
            buffer = StringManager::formatString(buffer, regs.eax, fmtArgs);
            regs = backup;
            regs.edi = X86Pointer(buffer);
            return 0;
        });

    registerHook(
        0x00438A6C,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Gui::init();
            regs = backup;
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
            auto rt = X86Pointer<Gfx::RenderTarget>(regs.edi);

            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            drawingCtx.pushRenderTarget(*rt);

            window->draw(drawingCtx);

            drawingCtx.popRenderTarget();

            regs = backup;
            return 0;
        });

    registerHook(
        0x004CF63B,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Gfx::renderAndUpdate();
            regs = backup;
            return 0;
        });

    Ui::ProgressBar::registerHooks();
    World::TileClearance::registerHooks();
    World::TileManager::registerHooks();
    World::AnimationManager::registerHooks();
    Ui::Windows::TextInput::registerHooks();
    Ui::Windows::ToolTip::registerHooks();
    Ui::Windows::BuildVehicle::registerHooks();
    Ui::Windows::Error::registerHooks();
    Ui::Windows::Construction::registerHooks();
    Ui::WindowManager::registerHooks();
    Ui::ViewportManager::registerHooks();
    GameCommands::registerHooks();
    Scenario::registerHooks();
    StationManager::registerHooks();
    TownManager::registerHooks();
    S5::registerHooks();
    Title::registerHooks();
    OpenLoco::Tutorial::registerHooks();
    Config::registerHooks();
    ObjectManager::registerHooks();
    Vehicles::registerHooks();

    // Part of 0x004691FA
    registerHook(
        0x0046956E,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            World::Pos2 pos(regs.ax, regs.cx);
            World::SurfaceElement* surface = X86Pointer<World::SurfaceElement>(regs.esi);

            World::WaveManager::createWave(*surface, pos);

            regs = backup;
            return 0;
        });

    registerHook(
        0x004AB655,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Vehicles::VehicleBase* v = X86Pointer<Vehicles::VehicleBase>(regs.esi);
            v->asVehicleBody()->secondaryAnimationUpdate();
            regs = backup;
            return 0;
        });

    registerHook(
        0x004392BD,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            Gui::resize();
            regs = backup;
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

            Gfx::render(regs.ax, regs.bx, regs.dx, regs.bp);

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

    registerHook(
        0x00448C79,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;

            Gfx::RenderTarget* rt = X86Pointer<Gfx::RenderTarget>(regs.edi);
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            drawingCtx.pushRenderTarget(*rt);
            drawingCtx.drawImage({ regs.cx, regs.dx }, ImageId::fromUInt32(regs.ebx));
            drawingCtx.popRenderTarget();

            regs = backup;
            return 0;
        });

    registerHook(
        0x00454A43,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            World::Pos2 pos{ regs.ax, regs.cx };
            uint8_t primaryWall = regs.bl;
            uint8_t secondaryWall = regs.bh;
            auto* industry = IndustryManager::get(static_cast<IndustryId>(regs.dh));
            industry->expandGrounds(pos, primaryWall, secondaryWall, regs.dl);
            regs = backup;
            return 0;
        });
}
