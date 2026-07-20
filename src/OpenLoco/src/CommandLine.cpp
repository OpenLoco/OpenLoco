#include "CommandLine.h"
#include "GameSaveCompare.h"
#include "GameState.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "S5/SawyerStream.h"
#include <OpenLoco/Core/MemoryStream.h>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Version.hpp>
#include <chrono>
#include <fmt/chrono.h>
#include <iostream>
#include <optional>
#include <stdlib.h>
#include <string_view>
#include <vector>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco
{
    static CommandLineOptions _cmdlineOptions;

    const CommandLineOptions& getCommandLineOptions()
    {
        return _cmdlineOptions;
    }

    void setCommandLineOptions(const CommandLineOptions& options)
    {
        _cmdlineOptions = options;
    }

    class CommandLineParser
    {
    private:
        std::vector<std::string_view> _args;
        std::vector<std::pair<std::string_view, size_t>> _registered;
        std::vector<std::pair<std::string_view, std::vector<std::string_view>>> _values;
        std::string _errorMessage;

        static bool isLongOption(std::string_view arg)
        {
            if (arg.size() >= 3 && arg[0] == '-' && arg[1] == '-')
            {
                return true;
            }
            return false;
        }

        static bool isShortOption(std::string_view arg)
        {
            if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-')
            {
                return true;
            }
            return false;
        }

        static bool isOptionTerminator(std::string_view arg)
        {
            return arg == "--";
        }

        static bool isArg(std::string_view arg)
        {
            return !isLongOption(arg) && !isShortOption(arg) && !isOptionTerminator(arg);
        }

        std::optional<size_t> getOptionArgCount(std::string_view arg)
        {
            for (const auto& r : _registered)
            {
                if (std::get<0>(r) == arg)
                {
                    return std::get<1>(r);
                }
            }
            return {};
        }

        void setOption(std::string_view arg)
        {
            for (auto& r : _values)
            {
                if (std::get<0>(r) == arg)
                {
                    return;
                }
            }
            _values.emplace_back(arg, std::vector<std::string_view>());
        }

        void appendValue(std::string_view arg, std::string_view value)
        {
            for (auto& r : _values)
            {
                if (std::get<0>(r) == arg)
                {
                    auto& v = std::get<1>(r);
                    v.push_back(value);
                    return;
                }
            }
            _values.emplace_back(arg, std::vector<std::string_view>{ value });
        }

        bool appendValues(std::string_view arg, size_t count, size_t& i)
        {
            for (size_t j = 0; j < count; j++)
            {
                i++;
                if (i < _args.size())
                {
                    auto& nextArg = _args[i];
                    if (isArg(nextArg))
                    {
                        appendValue(arg, nextArg);
                    }
                    else
                    {
                        setArgumentCountError(arg, count);
                        return false;
                    }
                }
                else
                {
                    setArgumentCountError(arg, count);
                    return false;
                }
            }
            return true;
        }

        void setArgumentUnknownError(std::string_view arg)
        {
            _errorMessage = std::string("Unknown option: ") + std::string(arg);
        }

        void setArgumentCountError(std::string_view arg, size_t count)
        {
            if (count == 1)
            {
                _errorMessage = std::string("Expected ") + "1 argument for " + std::string(arg);
            }
            else
            {
                _errorMessage = std::string("Expected ") + std::to_string(count) + " arguments for " + std::string(arg);
            }
        }

    public:
        CommandLineParser(const std::vector<std::string>& argv)
        {
            _args.resize(argv.size() - 1);
            for (size_t i = 1; i < argv.size(); i++)
            {
                _args[i - 1] = argv[i];
            }
        }

        std::string_view getErrorMessage() const
        {
            return _errorMessage;
        }

        CommandLineParser& registerOption(std::string_view option, size_t count = 0)
        {
            _registered.emplace_back(option, count);
            return *this;
        }

        CommandLineParser& registerOption(std::string_view a, std::string_view b, size_t count = 0)
        {
            _registered.emplace_back(a, count);
            _registered.emplace_back(b, count);
            return *this;
        }

        bool parse()
        {
            auto noMoreOptions = false;
            for (size_t i = 0; i < _args.size(); i++)
            {
                auto& arg = _args[i];
                if (!noMoreOptions && isLongOption(arg))
                {
                    auto count = getOptionArgCount(arg);
                    if (count)
                    {
                        if (*count == 0)
                        {
                            setOption(arg);
                        }
                        else if (!appendValues(arg, *count, i))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        setArgumentUnknownError(arg);
                        return false;
                    }
                }
                else if (!noMoreOptions && isShortOption(arg))
                {
                    for (auto c : arg.substr(1))
                    {
                        auto count = getOptionArgCount(arg);
                        if (count)
                        {
                            if (*count == 0)
                            {
                                setOption(arg);
                            }
                            else if (!appendValues(arg, *count, i))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            setArgumentUnknownError(std::string_view(&c, 1));
                            return false;
                        }
                    }
                }
                else if (!noMoreOptions && arg == "--")
                {
                    noMoreOptions = true;
                }
                else
                {
                    appendValue("", arg);
                }
            }
            return true;
        }

        bool hasOption(std::string_view name)
        {
            for (auto& r : _values)
            {
                if (std::get<0>(r) == name)
                {
                    return true;
                }
            }
            return false;
        }

        const std::vector<std::string_view>* getArgs(std::string_view name)
        {
            for (auto& r : _values)
            {
                if (std::get<0>(r) == name)
                {
                    return &std::get<1>(r);
                }
            }
            return nullptr;
        }

        std::string_view getArg(std::string_view name, size_t index = 0)
        {
            const auto* args = getArgs(name);
            if (args == nullptr || index >= args->size())
            {
                return {};
            }
            return (*args)[index];
        }

        std::string_view getArg(size_t index)
        {
            return getArg("", index);
        }

        template<typename int32_t>
        std::optional<int32_t> getArg(std::string_view name, size_t index = 0)
        {
            auto arg = getArg(name, index);
            if (arg.size() != 0)
            {
                return std::stoi(std::string(arg));
            }
            return {};
        }

        template<typename T>
        std::optional<T> getArg(size_t index)
        {
            return getArg<T>("", index);
        }
    };

    std::optional<CommandLineOptions> parseCommandLine(std::vector<std::string>&& argv)
    {
        auto parser = CommandLineParser(argv)
                          .registerOption("--bind", 1)
                          .registerOption("--port", "-p", 1)
                          .registerOption("-o", 1)
                          .registerOption("--help", "-h")
                          .registerOption("--version")
                          .registerOption("--intro")
                          .registerOption("--log_levels", 1)
                          .registerOption("--all", "-a")
                          .registerOption("--locomotion_path", 1);

        if (!parser.parse())
        {
            Logging::error("{}", parser.getErrorMessage());
            return {};
        }

        CommandLineOptions options;
        if (parser.hasOption("--version"))
        {
            options.action = CommandLineAction::version;
        }
        else if (parser.hasOption("--help") || parser.hasOption("-h"))
        {
            options.action = CommandLineAction::help;
        }
        else if (parser.hasOption("--intro"))
        {
            options.action = CommandLineAction::intro;
        }
        else
        {
            auto firstArg = parser.getArg(0);
            if (firstArg == "host")
            {
                options.action = CommandLineAction::host;
                options.path = parser.getArg(1);
            }
            else if (firstArg == "join")
            {
                options.action = CommandLineAction::join;
                options.address = parser.getArg(1);
            }
            else if (firstArg == "uncompress")
            {
                options.action = CommandLineAction::uncompress;
                options.path = parser.getArg(1);
            }
            else if (firstArg == "simulate")
            {
                options.action = CommandLineAction::simulate;
                options.path = parser.getArg(1);
                options.ticks = parser.getArg<int32_t>(2);
                options.path2 = parser.getArg(3);
            }
            else if (firstArg == "compare")
            {
                options.action = CommandLineAction::compare;
                if (parser.hasOption("--all") || parser.hasOption("-a"))
                {
                    options.all = "all";
                    options.path = parser.getArg(1);
                    options.path2 = parser.getArg(2);
                }
                else
                {
                    options.path = parser.getArg(1);
                    options.path2 = parser.getArg(2);
                }
            }
            else
            {
                options.path = parser.getArg(0);
            }
        }

        options.bind = parser.getArg("--bind");
        options.port = parser.getArg<int32_t>("--port");
        if (!options.port)
        {
            options.port = parser.getArg<int32_t>("-p");
        }
        options.outputPath = parser.getArg("-o");

        if (parser.hasOption("--log_levels"))
        {
            options.logLevels = parser.getArg("--log_levels");
        }
        else
        {
            options.logLevels = "info, warning, error";
        }

        if (parser.hasOption("--locomotion_path"))
        {
            options.locomotionDataPath = parser.getArg("--locomotion_path");
        }

        return options;
    }
}
