#include <cstdio>
#include <cstring>
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include "../environment.h"
#include "../graphics/gfx.h"
#include "../input.h"
#include "../log.h"
#include "../platform/platform.h"
#include "../station.h"
#include "../things/vehicle.h"
#include "../ui.h"
#include "../utility/string.hpp"
#include "../windowmgr.h"
#include "interop.hpp"

using namespace openloco;

#define STUB() console::log(__FUNCTION__)

#ifdef _MSC_VER
#define STDCALL __stdcall
#define CDECL __cdecl
#define FORCE_ALIGN_ARG_POINTER
#elif defined(__GNUC__)
#define STDCALL __attribute__((stdcall))
#define CDECL __attribute__((cdecl))
#define FORCE_ALIGN_ARG_POINTER __attribute__((force_align_arg_pointer))
#else
#error Unknown compiler, please define STDCALL and CDECL
#endif

#pragma warning(disable : 4505) // unreferenced local function has been removed.

static void STDCALL fn_40447f()
{
    STUB();
    return;
}

static void STDCALL fn_404b68(int a0, int a1, int a2, int a3)
{
    STUB();
    return;
}

static void STDCALL fn_404e8c()
{
    STUB();
    return;
}

static void STDCALL fn_404eac(int i1, int i2, int i3, int i4)
{
    STUB();
    return;
}

#pragma pack(push, 1)

struct palette_entry_t
{
    uint8_t b, g, r, a;
};
#pragma pack(pop)
using set_palette_func = void (*)(const palette_entry_t* palette, int32_t index, int32_t count);
static interop::loco_global<set_palette_func, 0x0052524C> set_palette_callback;

FORCE_ALIGN_ARG_POINTER
static void CDECL fn_4054a3(const palette_entry_t* palette, int32_t index, int32_t count)
{
    (*set_palette_callback)(palette, index, count);
}

FORCE_ALIGN_ARG_POINTER
static bool STDCALL fn_4054b9()
{
    STUB();
    return true;
}

FORCE_ALIGN_ARG_POINTER
static uint32_t STDCALL lib_timeGetTime()
{
    STUB();
    return platform::get_time();
}

//typedef bool (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, char*, char*, void*);
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

static void STDCALL fn_4078fe()
{
    STUB();
    return;
}

static void STDCALL fn_407b26()
{
    STUB();
    return;
}

///region Progress bar

static void CDECL fn_4080bb(char* lpWindowName, uint32_t a1)
{
    console::log("Create progress bar");
}

static void CDECL fn_408163()
{
    console::log("Destroy progress bar");
}

static void CDECL fn_40817b(uint16_t arg0)
{
    console::log("SendMessage(PBM_SETRANGE, %d)", arg0);
    console::log("SendMessage(PBM_SETSTEP, %d)", 1);
}

static void CDECL fn_4081ad(int32_t wParam)
{
    console::log("SendMessage(PBM_SETPOS, %d)", wParam);
}

///endregion

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekSet(FILE* a0, int32_t distance)
{
    console::log("seek %d bytes from current", distance);
    fseek(a0, distance, SEEK_SET);
    return ftell(a0);
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekFromCurrent(FILE* a0, int32_t distance)
{
    console::log("seek %d bytes from current", distance);
    fseek(a0, distance, SEEK_CUR);
    return ftell(a0);
}

FORCE_ALIGN_ARG_POINTER
static uint32_t CDECL fn_FileSeekFromEnd(FILE* a0, int32_t distance)
{
    console::log("seek %d bytes from end", distance);
    fseek(a0, distance, SEEK_END);
    return ftell(a0);
}

FORCE_ALIGN_ARG_POINTER
static int32_t CDECL fn_FileRead(FILE* a0, char* buffer, int32_t size)
{
    console::log("read %d bytes (%d)", size, fileno(a0));
    size = fread(buffer, 1, size, a0);

    return size;
}

FORCE_ALIGN_ARG_POINTER
static int CDECL fn_CloseHandle(FILE* file)
{
    // STUB();
    if (file == nullptr)
    {
        return 1;
    }

    fclose(file);
    return 0;
}

FORCE_ALIGN_ARG_POINTER
static FILE* CDECL fn_CreateFile(char* lpFileName)
{
    FILE* pFILE = fopen(lpFileName, "r");
    console::log("%s %s (%d)", __FUNCTION__, lpFileName, fileno(pFILE));
    return pFILE;
    // return CreateFile(lpFileName, 0x80000000, FILE_SHARE_READ,NULL, OPEN_EXISTING, 0x10000080, 0);
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
    std::vector<openloco::environment::fs::path> fileList;
};

FORCE_ALIGN_ARG_POINTER
static Session* CDECL fn_FindFirstFile(char* lpFileName, FindFileData* out)
{
    console::log("%s (%s)", __FUNCTION__, lpFileName);

    Session* data = new Session;

    openloco::environment::fs::path path = lpFileName;
#ifdef _OPENLOCO_USE_BOOST_FS_
    std::string format = path.filename().string();
#else
    std::string format = path.filename().u8string();
#endif
    path.remove_filename();

    openloco::environment::fs::directory_iterator iter(path), end;

    while (iter != end)
    {
        data->fileList.push_back(iter->path());
        ++iter;
    }

#ifdef _OPENLOCO_USE_BOOST_FS_
    utility::strcpy_safe(out->cFilename, data->fileList[0].filename().string().c_str());
#else
    utility::strcpy_safe(out->cFilename, data->fileList[0].filename().u8string().c_str());
#endif
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

#ifdef _OPENLOCO_USE_BOOST_FS_
    utility::strcpy_safe(out->cFilename, data->fileList[0].filename().string().c_str());
#else
    utility::strcpy_safe(out->cFilename, data->fileList[0].filename().u8string().c_str());
#endif
    data->fileList.erase(data->fileList.begin());

    return true;
}

static void CDECL fn_FindClose(Session* data)
{
    STUB();

    delete data;
}

static void CDECL fn_4078b5(void)
{
    STUB();
}

static void CDECL fnc0(void)
{
    STUB();
}

static void CDECL fnc1(int i1)
{
    STUB();
}

static void CDECL fnc2(int i1, int i2)
{
    STUB();
}

static void STDCALL fn0()
{
    return;
}

static void STDCALL fn1(int i1)
{
    return;
}

static void STDCALL fn2(int i1, int i2)
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

STDCALL
void fn_dump(uint32_t address)
{
    console::log("Missing hook: 0x%x", address);
}

enum
{
    DS_OK = 0,
    DSERR_NODRIVER = 0x88780078,
};

uint32_t STDCALL lib_DirectSoundCreate(void* lpGuid, void* ppDS, void* pUnkOuter)
{
    console::log("lib_DirectSoundCreate(%lx, %lx, %lx)", (uintptr_t)lpGuid, (uintptr_t)ppDS, (uintptr_t)pUnkOuter);

    return DSERR_NODRIVER;
}

uint32_t STDCALL lib_CreateRectRgn(int x1, int y1, int x2, int y2)
{
    console::log("CreateRectRgn(%d, %d, %d, %d)", x1, y1, x2, y2);
    return 0;
}

uint STDCALL lib_GetUpdateRgn(uintptr_t hWnd, uintptr_t hRgn, bool bErase)
{
    console::log("GetUpdateRgn(%lx, %lx, %d)", hWnd, hRgn, bErase);
    return 0;
}

void* STDCALL lib_OpenMutexA(uint32_t dwDesiredAccess, bool bInheritHandle, char* lpName)
{
    console::log("OpenMutexA(0x%x, %d, %s)", dwDesiredAccess, bInheritHandle, lpName);

    return nullptr;
}

bool STDCALL lib_DeleteFileA(char* lpFileName)
{
    console::log("DeleteFileA(%s)", lpFileName);

    return false;
}

bool STDCALL lib_WriteFile(
    FILE* hFile,
    char* buffer,
    size_t nNumberOfBytesToWrite,
    uint32_t* lpNumberOfBytesWritten,
    uintptr_t lpOverlapped)
{
    console::log("WriteFile(%s)", buffer);

    return true;
}

void* STDCALL FORCE_ALIGN_ARG_POINTER
lib_CreateFileA(
    char* lpFileName,
    uint32_t dwDesiredAccess,
    uint32_t dwShareMode,
    uintptr_t lpSecurityAttributes,
    uint32_t dwCreationDisposition,
    uint32_t dwFlagsAndAttributes,
    uintptr_t hTemplateFile)
{
    console::log("CreateFile(%s, %x)", lpFileName, dwDesiredAccess);

    return fopen(lpFileName, "r");
}

FORCE_ALIGN_ARG_POINTER
bool STDCALL lib_SetFileAttributesA(char* lpFileName, uint32_t dwFileAttributes)
{
    // FILE_ATTRIBUTE_NORMAL = 0x80
    assert(dwFileAttributes == 0x80);
    console::log("SetFileAttributes(%s, %x)", lpFileName, dwFileAttributes);

    if (access(lpFileName, R_OK | W_OK) != -1)
    {
        // File exists
        return true;
    }

    // FIXME: create file if doesnt exist
    assert(false);
    return false;
}

void* STDCALL lib_CreateMutexA(uintptr_t lmMutexAttributes, bool bInitialOwner, char* lpName)
{
    console::log("CreateMutexA(0x%lx, %d, %s)", lmMutexAttributes, bInitialOwner, lpName);

    return nullptr;
}

void STDCALL lib_CloseHandle(int a0)
{
    console::log("CloseHandle(%d)", a0);
}

FORCE_ALIGN_ARG_POINTER
void STDCALL lib_PostQuitMessage(int32_t exitCode)
{
    console::log("lib_PostQuitMessage(%d)", exitCode);
    exit(exitCode);
}

void STDCALL lib_4(int div, int addr, int a0, int a1, int a2, int a3)
{
    console::log("::::::::::::::::::: %x: %x %x %x %x", addr, a0, a1, a2, a3);
}

static void register_no_win32_hooks()
{
    using namespace openloco::interop;

    write_jmp(0x40447f, (void*)&fn_40447f);
    register_hook_stub(0x40726d);
    write_jmp(0x404b68, (void*)&fn_404b68);
    write_jmp(0x404e8c, (void*)&fn_404e8c);
    //    write_jmp(0x404eac, (void*)&fn_404eac);
    write_jmp(0x4054b9, (void*)&fn_4054b9);
    write_jmp(0x4064fa, (void*)&fn0);
    write_jmp(0x4d1401, (void*)&fn_malloc);
    write_jmp(0x4D1B28, (void*)&fn_realloc);
    write_jmp(0x4D1355, (void*)&fn_free);
    write_jmp(0x4054a3, (void*)&fn_4054a3);
    write_jmp(0x4072ec, (void*)&fn0);
    //write_jmp(0x4078b5, (void*)&fn_4078b5);
    write_jmp(0x4078be, (void*)&fn_4078be);
    //    write_jmp(0x4078f8, (void*)&fn_timeGetTime);
    write_jmp(0x4078fe, (void*)&fn_4078fe);
    write_jmp(0x407b26, (void*)&fn_407b26);
    write_jmp(0x4080bb, (void*)&fn_4080bb);
    write_jmp(0x408163, (void*)&fn_408163);
    write_jmp(0x40817b, (void*)&fn_40817b);
    write_jmp(0x4081ad, (void*)&fn_4081ad);
    write_jmp(0x4081c5, (void*)&fn_FileSeekSet);
    write_jmp(0x4081d8, (void*)&fn_FileSeekFromCurrent);
    write_jmp(0x4081eb, (void*)&fn_FileSeekFromEnd);
    write_jmp(0x4081fe, (void*)&fn_FileRead);
    write_jmp(0x408297, (void*)&fn_CloseHandle);
    write_jmp(0x4082ad, (void*)&fn_CreateFile);
    //write_jmp(0x4082e6, (void*)&fnc1);
    //write_jmp(0x4082f8, (void*)&fnc2);
    write_jmp(0x40830e, (void*)&fn_FindFirstFile);
    write_jmp(0x40831d, (void*)&fn_FindNextFile);
    write_jmp(0x40832c, (void*)&fn_FindClose);
    write_jmp(0x4d0fac, (void*)&fn_DirectSoundEnumerateA);

    // sound
    register_hook_stub(0x489cb5);
    register_hook_stub(0x489f1b);
    register_hook_stub(0x48a4bf);

    // fill DLL hooks for ease of debugging
    for (int i = 0x4d7000; i <= 0x4d72d8; i += 4)
    {
        hook_dump(i, (void*)&fn_dump);
    }

    // dsound.dll
    hook_lib(0x4d7024, (void*)&lib_DirectSoundCreate);

    // gdi32.dll
    hook_lib(0x4d7078, (void*)&lib_CreateRectRgn);

    // kernel32.dll
    hook_lib(0x4d70e0, (void*)&lib_CreateMutexA);
    hook_lib(0x4d70e4, (void*)&lib_OpenMutexA);
    hook_lib(0x4d70f0, (void*)&lib_WriteFile);
    hook_lib(0x4d70f4, (void*)&lib_DeleteFileA);
    hook_lib(0x4d70f8, (void*)&lib_SetFileAttributesA);
    hook_lib(0x4d70fC, (void*)&lib_CreateFileA);

    // user32.dll
    hook_lib(0x4d71e8, (void*)&lib_PostQuitMessage);
    hook_lib(0x4d714c, (void*)&lib_CloseHandle);
    hook_lib(0x4d7248, (void*)&lib_GetUpdateRgn);
    hook_lib(0x4d72b0, (void*)&lib_timeGetTime);
}

void openloco::interop::register_hooks()
{
    using namespace openloco::ui::windows;

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

#ifdef _NO_LOCO_WIN32_
    register_no_win32_hooks();
#endif

    register_hook(
        0x004416B5,
        [](registers& regs) -> uint8_t {
            using namespace openloco::environment;

            auto buffer = (char*)0x009D0D72;
            auto path = get_path((path_id)regs.ebx);
#ifdef _OPENLOCO_USE_BOOST_FS_
            // TODO: use utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(buffer, path.make_preferred().string().c_str());
#else
            // TODO: use utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(buffer, path.make_preferred().u8string().c_str());
#endif
            regs.ebx = (int32_t)buffer;
            return 0;
        });

    // Replace ui::update() with our own
    register_hook(
        0x004524C1,
        [](registers& regs) -> uint8_t {
            ui::update();
            return 0;
        });

    register_hook(
        0x00407BA3,
        [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            auto cursor = (ui::cursor_id)regs.edx;
            ui::set_cursor(cursor);
            return 0;
        });
    register_hook(
        0x004CF142,
        [](registers& regs) -> uint8_t {
            ui::set_cursor(ui::cursor_id::blank);
            return 0;
        });

    register_hook(
        0x00445AB9,
        [](registers& regs) -> uint8_t {
            auto result = prompt_browse(
                (browse_type)regs.al,
                (char*)regs.ecx,
                (const char*)regs.edx,
                (const char*)regs.ebx);
            regs.eax = result ? 1 : 0;
            return 0;
        });

    register_hook(
        0x00446F6B,
        [](registers& regs) -> uint8_t {
            auto result = prompt_ok_cancel(regs.eax);
            regs.eax = result ? 1 : 0;
            return 0;
        });

    register_hook(
        0x00407218,
        [](registers& regs) -> uint8_t {
            openloco::input::sub_407218();
            return 0;
        });
    register_hook(
        0x00407231,
        [](registers& regs) -> uint8_t {
            openloco::input::sub_407231();
            return 0;
        });

    register_hook(
        0x00492793,
        [](registers& regs) -> uint8_t {
            auto station = (openloco::station*)regs.esi;
            regs.al = (station->update_cargo() != 0);
            return 0;
        });

    register_hook(
        0x0049D3F6,
        [](registers& regs) -> uint8_t {
            ui::windows::construction_mouse_up(*((ui::window*)regs.esi), regs.dx);
            return 0;
        });

    register_hook(
        0x0048ED2F,
        [](registers& regs) -> uint8_t {
            ui::windows::station_2_scroll_paint(
                *((ui::window*)regs.esi),
                *((gfx::drawpixelinfo_t*)regs.edi));
            return 0;
        });

    register_hook(
        0x00498E9B,
        [](registers& regs) -> uint8_t {
            openloco::ui::windows::sub_498E9B((openloco::ui::window*)regs.esi);
            return 0;
        });

    register_hook(
        0x004BA8D4,
        [](registers& regs) -> uint8_t {
            auto v = (openloco::vehicle*)regs.esi;
            v->sub_4BA8D4();
            return 0;
        });

    // Remove the set window pos function, we do not want it as it
    // keeps moving the process window to 0, 0
    // Can be removed when windowmgr:update() is hooked
    write_ret(0x00406520);

    // Remove check for is road in use when removing roads. It is
    // quite annoying when it's sometimes only the player's own
    // vehicles that are using it.
    write_nop(0x004776DD, 6);
}
