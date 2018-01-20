#include <dirent.h>
#include <cstring>
#include "../environment.h"
#include "../graphics/gfx.h"
#include "../input.h"
#include "../station.h"
#include "../things/vehicle.h"
#include "../ui.h"
#include "../windowmgr.h"
#include "interop.hpp"

using namespace openloco;

__attribute__((stdcall))
static void
fn_40447f()
{
    printf("%s\n", __FUNCTION__);
    return;
}

__attribute__((stdcall))
static void
fn_404e8c()
{
    printf("%s\n", __FUNCTION__);
    return;
}

__attribute__((stdcall))
static void
fn_404eac(int i1, int i2, int i3, int i4)
{
    printf("%s\n", __FUNCTION__);
    return;
}

__attribute__((stdcall))
static void
fn_4054b9()
{
    printf("%s\n", __FUNCTION__);
    return;
}

__attribute__((stdcall))
static long
fn_timeGetTime()
{
    printf("%s\n", __FUNCTION__);
    return 0;
}

//typedef bool (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, char*, char*, void*);
__attribute__((stdcall))
static long
fn_DirectSoundEnumerateA(void *pDSEnumCallback, void *pContext)
{
    printf("%s\n", __FUNCTION__);
    return 0;
}

__attribute__((stdcall))
static void
fn_4078be()
{
    printf("%s\n", __FUNCTION__);
    return;
}

__attribute__((stdcall))
static void
fn_4078fe()
{
    printf("%s\n", __FUNCTION__);
    return;
}

__attribute__((stdcall))
static void
fn_407b26()
{
    printf("%s\n", __FUNCTION__);
    return;
}

///region Progress bar

__attribute__((cdecl))
static void
fn_4080bb(char *lpWindowName, uint32_t a1)
{
    printf("Create progress bar\n");
}

__attribute__((cdecl))
static void
fn_408163()
{
    printf("Destroy progress bar\n");
}

__attribute__((cdecl))
static void
fn_40817b(uint16_t arg0)
{
    printf("SendMessage(PBM_SETRANGE, %d)\n", arg0);
    printf("SendMessage(PBM_SETSTEP, %d)\n", 1);
}

__attribute__((cdecl))
static void
fn_4081ad(int32_t wParam)
{
    printf("SendMessage(PBM_SETPOS, %d)\n", wParam);
}

///endregion

__attribute__((cdecl))
__attribute__ ((force_align_arg_pointer))
static void
fn_FileSeekFromEnd(FILE *a0, int32_t distance)
{
    printf("seek %d bytes from end\n", distance);
    fseek(a0, distance, SEEK_END);
}

__attribute__((cdecl))
__attribute__ ((force_align_arg_pointer))
static int32_t
fn_FileRead(FILE *a0, char *buffer, int32_t size)
{
    printf("read %d bytes\n", size);
    fread(buffer, 1, size, a0);

    return size;
}

__attribute__((cdecl))
__attribute__ ((force_align_arg_pointer))
static int
fn_CloseHandle(FILE *file)
{
    printf("%s\n", __FUNCTION__);
    if (file==nullptr)
    {
        return 1;
    }

    return fclose(file);
}

__attribute__((cdecl))
__attribute__ ((force_align_arg_pointer))
static FILE *
fn_CreateFile(char *lpFileName)
{
    printf("%s %s\n", __FUNCTION__, lpFileName);
    return fopen(lpFileName, "r");
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

__attribute__((cdecl))
__attribute__ ((force_align_arg_pointer))
static Session *
fn_FindFirstFile(char *lpFileName, FindFileData *out)
{
    printf("%s (%s)\n", __FUNCTION__,lpFileName );

    Session *data = new Session;

    openloco::environment::fs::path path = lpFileName;
#ifdef OPENLOCO_USE_BOOST_FS
    std::string format = path.filename().string();
#else
    std::string format = path.filename().u8string();
#endif
    path.remove_filename();

    openloco::environment::fs::directory_iterator iter(path), end;

    while (iter!=end)
    {
        data->fileList.push_back(iter->path());
        ++iter;
    }

#ifdef OPENLOCO_USE_BOOST_FS
    strcpy(out->cFilename, data->fileList[0].filename().string().c_str());
#else
    strcpy(out->cFilename, data->fileList[0].filename().u8string().c_str());
#endif
    data->fileList.erase(data->fileList.begin());
    return data;
}

__attribute__((cdecl))
static bool
fn_FindNextFile(Session *data, FindFileData *out)
{
    printf("%s\n", __FUNCTION__);

    if (data->fileList.size()==0)
    {
        return false;
    }

#ifdef OPENLOCO_USE_BOOST_FS
    strcpy(out->cFilename, data->fileList[0].filename().string().c_str());
#else
    strcpy(out->cFilename, data->fileList[0].filename().u8string().c_str());
#endif
    data->fileList.erase(data->fileList.begin());

    return true;
}

__attribute__((cdecl))
static void
fn_FindClose(Session *data)
{
    printf("%s\n", __FUNCTION__);

    delete data;
}

__attribute__((cdecl))
__attribute__ ((force_align_arg_pointer))
static void *
fn_406bf7(int arg0)
{
    printf("%s %d\n", __FUNCTION__, arg0);

    return malloc(arg0);
}

__attribute__((cdecl))
static void
fn_4078b5(void)
{
    printf("%s\n", __FUNCTION__);
}

__attribute__((cdecl))
static void
fnc0(void)
{
    printf("%s\n", __FUNCTION__);
}

__attribute__((cdecl))
static void
fnc1(int i1)
{
    printf("%s\n", __FUNCTION__);
}

__attribute__((cdecl))
static void
fnc2(int i1, int i2)
{
    printf("%s\n", __FUNCTION__);
}

__attribute__((stdcall))
static void
fn0()
{
    return;
}

__attribute__((stdcall))
static void
fn1(int i1)
{
    return;
}

__attribute__((stdcall))
static void
fn2(int i1, int i2)
{
    printf("%s\n", __FUNCTION__);
}

__attribute__((stdcall))
__attribute__ ((force_align_arg_pointer))
static void *
fn_malloc(uint32_t size)
{
    //printf("malloc %x\n", (uint32_t)size);
    void *ptr = malloc(size);
    return ptr;
}

__attribute__((stdcall))
__attribute__ ((force_align_arg_pointer))
static void *
fn_realloc(void *src, uint32_t size)
{
    //printf("malloc %x\n", (uint32_t)size);
    void *ptr = realloc(src, size);
    return ptr;
}


void openloco::interop::register_hooks()
{
    using namespace openloco::ui::windows;

    register_hook(0x004416B5,
                  [](registers &regs) -> uint8_t
                  {
                      using namespace openloco::environment;

                      auto buffer = (char *)0x009D0D72;
                      auto path = get_path((path_id)regs.ebx);
#ifdef OPENLOCO_USE_BOOST_FS
                      std::strcpy(buffer, path.make_preferred().string().c_str());
#else
                      std::strcpy(buffer, path.make_preferred().u8string().c_str());
#endif
                      regs.ebx = (int32_t)buffer;
                      return 0;
                  });

    hook_stdcall(0x40447f, (void *) &fn_40447f);
    hook_stdcall(0x404cd3, (void *) &fnc1);
    hook_stdcall(0x404e8c, (void *) &fn_404e8c);
    hook_stdcall(0x404eac, (void *) &fn_404eac);
    hook_stdcall(0x4054b9, (void *) &fn_4054b9);
    hook_stdcall(0x4064fa, (void *) &fn0);
    hook_stdcall(0x406bf7, (void *) &fn_406bf7);
    hook_stdcall(0x406c02, (void *) &fn_realloc);
    hook_stdcall(0x4072ec, (void *) &fn0);
    hook_stdcall(0x4072ec, (void *) &fn0);
    hook_stdcall(0x4078b5, (void *) &fn_4078b5);
    hook_stdcall(0x4078be, (void *) &fn_4078be);
    hook_stdcall(0x4078f8, (void *) &fn_timeGetTime);
    hook_stdcall(0x4078fe, (void *) &fn_4078fe);
    hook_stdcall(0x407b26, (void *) &fn_407b26);
    hook_stdcall(0x4080bb, (void *) &fn_4080bb);
    hook_stdcall(0x408163, (void *) &fn_408163);
    hook_stdcall(0x40817b, (void *) &fn_40817b);
    hook_stdcall(0x4081ad, (void *) &fn_4081ad);
    hook_stdcall(0x4081eb, (void *) &fn_FileSeekFromEnd);
    hook_stdcall(0x4081fe, (void *) &fn_FileRead);
    hook_stdcall(0x408297, (void *) &fn_CloseHandle);
    hook_stdcall(0x4082ad, (void *) &fn_CreateFile);
    hook_stdcall(0x4082e6, (void *) &fnc1);
    hook_stdcall(0x4082f8, (void *) &fnc2);
    hook_stdcall(0x40830e, (void *) &fn_FindFirstFile);
    hook_stdcall(0x40831d, (void *) &fn_FindNextFile);
    hook_stdcall(0x40832c, (void *) &fn_FindClose);


#define REG(x)    register_hook(x, \
                  [](registers &regs) -> uint8_t \
                  { \
                      printf("                    fn %x\n", x); \
                      return 0; \
                  })

    REG(0x431695);
    REG(0x473a95);
    REG(0x4Cf456);

    hook_stdcall(0x4d0fac, (void *) &fn_DirectSoundEnumerateA);
    hook_stdcall(0x4d1401, (void *) &fn_malloc);



    // Replace ui::update() with our own
    register_hook(0x004524C1,
                  [](registers &regs) -> uint8_t
                  {
                      ui::update();
                      return 0;
                  });

    register_hook(0x00407BA3,
                  [](registers &regs) -> uint8_t
                  {
                      auto cursor = (ui::cursor_id)regs.edx;
                      ui::set_cursor(cursor);
                      return 0;
                  });
    register_hook(0x004CF142,
                  [](registers &regs) -> uint8_t
                  {
                      ui::set_cursor(ui::cursor_id::blank);
                      return 0;
                  });

    register_hook(0x00445AB9,
                  [](registers &regs) -> uint8_t
                  {
                      auto result = prompt_browse(
                          (browse_type)regs.al,
                          (char *)regs.ecx,
                          (const char *)regs.edx,
                          (const char *)regs.ebx);
                      regs.eax = result ? 1 : 0;
                      return 0;
                  });

    register_hook(0x00446F6B,
                  [](registers &regs) -> uint8_t
                  {
                      auto result = prompt_ok_cancel(regs.eax);
                      regs.eax = result ? 1 : 0;
                      return 0;
                  });

    register_hook(0x00407218,
                  [](registers &regs) -> uint8_t
                  {
                      openloco::input::sub_407218();
                      return 0;
                  });
    register_hook(0x00407231,
                  [](registers &regs) -> uint8_t
                  {
                      openloco::input::sub_407231();
                      return 0;
                  });

    register_hook(0x00492793,
                  [](registers &regs) -> uint8_t
                  {
                      auto station = (openloco::station *)regs.esi;
                      regs.al = (station->update_cargo() != 0);
                      return 0;
                  });

    register_hook(0x0049D3F6,
                  [](registers &regs) -> uint8_t
                  {
                      ui::windows::construction_mouse_up(*((ui::window *)regs.esi), regs.dx);
                      return 0;
                  });

    register_hook(0x0048ED2F,
                  [](registers &regs) -> uint8_t
                  {
                      ui::windows::station_2_scroll_paint(
                          *((ui::window *)regs.esi),
                          *((gfx::drawpixelinfo_t *)regs.edi));
                      return 0;
                  });

    register_hook(0x00498E9B,
                  [](registers &regs) -> uint8_t
                  {
                      openloco::ui::windows::sub_498E9B((openloco::ui::window *)regs.esi);
                      return 0;
                  });

    register_hook(0x004BA8D4,
                  [](registers &regs) -> uint8_t
                  {
                      auto v = (openloco::vehicle *)regs.esi;
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
