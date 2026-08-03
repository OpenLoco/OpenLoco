#if defined(__EMSCRIPTEN__)

#include "Platform.h"
#include <cstdlib>
#include <iostream>
#include <emscripten.h>

namespace OpenLoco::Platform
{
    void initialise()
    {
    }

    uint32_t getTime()
    {
        // Use Emscripten's JavaScript time functions
        return static_cast<uint32_t>(emscripten_get_now());
    }

    std::vector<fs::path> getDrives()
    {
        // WebAssembly doesn't have the concept of drives
        return {};
    }

    std::string getEnvironmentVariable(const std::string& name)
    {
        auto result = getenv(name.c_str());
        return result == nullptr ? std::string() : result;
    }

    fs::path getUserDirectory()
    {
        // In Emscripten, use the MEMFS root or a sensible default
        // Emscripten's file system is virtual, so we use a standard path
        auto path = fs::path(getEnvironmentVariable("HOME"));
        if (path.empty())
        {
            // Default to root of MEMFS
            path = "/";
        }
        return path / fs::path(".openloc");
    }

    fs::path getCurrentExecutablePath()
    {
        // In WebAssembly, there's no traditional executable path
        // Return a placeholder that indicates we're running in a browser
        return "/openloc.wasm";
    }

    fs::path promptDirectory([[maybe_unused]] const std::string& title, [[maybe_unused]] void* hwnd)
    {
        // In Emscripten, we can't prompt for directories in the traditional way
        // This would need to be handled via JavaScript interop
        std::cerr << "Directory prompting is not supported in WebAssembly. Using default path." << std::endl;
        return getUserDirectory();
    }

    bool isRunningInWine()
    {
        return false;
    }

    bool isStdOutRedirected()
    {
        // In Emscripten, stdout behavior depends on the environment
        // For now, assume it's not redirected (browser console)
        return false;
    }

    static bool hasTerminalVT100SupportImpl()
    {
        // Check for NO_COLOR environment variable
        const auto noColorEnvVar = getEnvironmentVariable("NO_COLOR");
        if (!noColorEnvVar.empty())
        {
            return false;
        }

        // Check TERM environment variable
        const auto termEnvVar = getEnvironmentVariable("TERM");
        if (termEnvVar.empty())
        {
            // In browser environments, we often don't have TERM set
            // but the console may still support colors
            return true;
        }

        return termEnvVar.compare("xterm") == 0
            || termEnvVar.compare("xterm-256color") == 0
            || termEnvVar.compare("rxvt-unicode-256color") == 0;
    }

    bool hasTerminalVT100Support()
    {
        static bool hasVT100Support = hasTerminalVT100SupportImpl();
        return hasVT100Support;
    }

    bool enableVT100TerminalMode()
    {
        if (isStdOutRedirected())
        {
            return false;
        }

        if (!hasTerminalVT100Support())
        {
            return false;
        }

        return true;
    }

    std::vector<std::string> getCmdLineVector(int argc, const char** argv)
    {
        std::vector<std::string> argvStrs;
        argvStrs.resize(argc);
        for (auto i = 0; i < argc; ++i)
        {
            argvStrs[i] = argv[i];
        }
        return argvStrs;
    }

    bool lockSingleInstance()
    {
        // In WebAssembly, the concept of multiple instances is different
        // Each browser tab/window is isolated, so we always return true
        // If you need to prevent multiple tabs, you'd need to use SharedArrayBuffer
        // or localStorage with polling, which is beyond the scope of this implementation
        return true;
    }
}

#endif
