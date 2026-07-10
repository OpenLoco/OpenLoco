#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace OpenLoco
{
    enum class CommandLineAction
    {
        none,
        host,
        join,
        uncompress,
        simulate,
        compare,
        help,
        version,
        intro,
    };

    struct CommandLineOptions
    {
        CommandLineAction action = CommandLineAction::none;
        std::string address;
        std::string path;
        std::string path2;
        std::optional<int32_t> ticks;
        std::string outputPath;
        std::string bind;
        std::optional<uint16_t> port{};
        std::string logLevels;
        std::string all;
        std::optional<std::string> locomotionDataPath{};
    };

    std::optional<CommandLineOptions> parseCommandLine(std::vector<std::string>&& argv);
    std::optional<int> runCommandLineOnlyCommand(const CommandLineOptions& options);
    const CommandLineOptions& getCommandLineOptions();
    void setCommandLineOptions(const CommandLineOptions& options);

    void printVersion();
    void printHelp();
}
