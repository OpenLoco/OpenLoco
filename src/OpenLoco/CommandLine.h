#pragma once

#include "Core/Optional.hpp"
#include <string>

namespace OpenLoco
{
    enum class CommandLineAction
    {
        none,
        uncompress,
        simulate,
        help,
        version,
        intro,
    };

    struct CommandLineOptions
    {
        CommandLineAction action;
        std::string path;
        std::optional<int32_t> ticks;
        std::string outputPath;
    };

    std::optional<CommandLineOptions> parseCommandLine(int argc, const char** argv);
    std::optional<CommandLineOptions> parseCommandLine(const char* args);
    std::optional<int> runCommandLineOnlyCommand(const CommandLineOptions& options);
    const CommandLineOptions& getCommandLineOptions();
    void setCommandLineOptions(const CommandLineOptions& options);

    void printVersion();
    void printHelp();
}
