#pragma once

#include <optional>
#include <string>

namespace OpenLoco
{
    enum class CommandLineAction
    {
        none,
        host,
        join,
        uncompress,
        simulate,
        help,
        version,
        intro,
    };

    struct CommandLineOptions
    {
        CommandLineAction action = CommandLineAction::none;
        std::string address;
        std::string path;
        std::optional<int32_t> ticks;
        std::string outputPath;
        std::string bind;
        std::optional<uint16_t> port{};
        std::string logLevels;
    };

    std::optional<CommandLineOptions> parseCommandLine(int argc, const char** argv);
    std::optional<CommandLineOptions> parseCommandLine(const char* args);
    std::optional<int> runCommandLineOnlyCommand(const CommandLineOptions& options);
    const CommandLineOptions& getCommandLineOptions();
    void setCommandLineOptions(const CommandLineOptions& options);

    void printVersion();
    void printHelp();
}
