#include "OpenLoco.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <OpenLoco/Diagnostics/Logging.h>
#include <windows.h>

#include <shellapi.h>

using namespace OpenLoco::Diagnostics;

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
    if (argw == nullptr)
    {
        Logging::error("CommandLineToArgvW failed to convert commandline");
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
            Logging::error("Failed to get cmdline argument utf8 length.");
            continue;
        }
        argvStrs[i].resize(length);

        if (WideCharToMultiByte(CP_UTF8, 0, argw[i], -1, argvStrs[i].data(), length, NULL, NULL) == 0)
        {
            Logging::error("Failed to convert cmdline argument to utf8.");
        }
    }
    LocalFree(reinterpret_cast<HLOCAL>(argw));
    const auto res = OpenLoco::main(std::move(argvStrs));

    return res;
}
#else
#include "Interop/Hooks.h"

int main(int argc, const char** argv)
{
    OpenLoco::Interop::loadSections();
    std::vector<std::string> argvStrs;
    argvStrs.resize(argc);
    for (auto i = 0; i < argc; ++i)
    {
        argvStrs[i] = argv[i];
    }
    return OpenLoco::main(std::move(argvStrs));
}
#endif
