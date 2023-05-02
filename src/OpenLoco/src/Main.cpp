#include "OpenLoco.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <shellapi.h>
/**
 * The function that is called directly from the host application (loco.exe)'s WinMain. This will be removed when OpenLoco can
 * be built as a stand alone application.
 */
// Hack to trick mingw into thinking we forward-declared this function.
extern "C" __declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
extern "C" __declspec(dllexport) int StartOpenLoco([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    // We need to get the Wide command line and convert it to utf8 command line parameters
    const auto cmdline = GetCommandLineW();

    int argc{};
    auto* argw = CommandLineToArgvW(cmdline, &argc);

    // This will hold a utf8 string of the command line
    // we are doing this convoluted method so that memory is
    // cleaned up automatically
    std::vector<std::string> argvStrs;
    argvStrs.resize(argc);
    // This will hold a utf8 c string of the command line
    auto argv = std::make_unique<char*[]>(argc);

    for (auto i = 0; i < argc; ++i)
    {
        int length = WideCharToMultiByte(CP_UTF8, 0, argw[i], -1, 0, 0, NULL, NULL);
        argvStrs[i].resize(length);
        argv.get()[i] = argvStrs[i].data();

        WideCharToMultiByte(CP_UTF8, 0, argw[i], -1, argvStrs[i].data(), length, NULL, NULL);
    }
    LocalFree(reinterpret_cast<HLOCAL>(argw));
    const auto res = OpenLoco::main(argc, const_cast<const char**>(argv.get()));

    return res;
}
#else
#include "Interop/Hooks.h"

int main(int argc, const char** argv)
{
    OpenLoco::Interop::loadSections();
    return OpenLoco::main(argc, argv);
}
#endif
