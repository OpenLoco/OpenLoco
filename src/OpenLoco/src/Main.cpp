#include "OpenLoco.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
/**
 * The function that is called directly from the host application (loco.exe)'s WinMain. This will be removed when OpenLoco can
 * be built as a stand alone application.
 */
// Hack to trick mingw into thinking we forward-declared this function.
extern "C" __declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
extern "C" __declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return OpenLoco::main(lpCmdLine);
}
#else
#include "Interop/Hooks.h"

int main(int argc, const char** argv)
{
    OpenLoco::Interop::loadSections();
    return OpenLoco::main(argc, argv);
}
#endif
