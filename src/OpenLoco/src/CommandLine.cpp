#include "CommandLine.h"
#include "GameSaveCompare.h"
#include "GameState.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "S5/SawyerStream.h"
#include <OpenLoco/Core/MemoryStream.h>
#include <OpenLoco/Diagnostics/Logging.h>
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

    static int uncompressFile(const CommandLineOptions& options);
    static int simulate(const CommandLineOptions& options);
    static int compare(const CommandLineOptions& options);

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

    void printVersion()
    {
        auto versionInfo = OpenLoco::getVersionInfo();
        std::cout << versionInfo << std::endl;
    }

    void printHelp()
    {
        std::cout << "usage: openloco [options] [path]" << std::endl;
        std::cout << "                host [options] <path>" << std::endl;
        std::cout << "                join [options] <address>" << std::endl;
        std::cout << "                uncompress [options] <path>" << std::endl;
        std::cout << "                simulate [options] <path> <ticks> [path]" << std::endl;
        std::cout << "                compare [options] <path1> <path2>" << std::endl;
        std::cout << std::endl;
        std::cout << "options:" << std::endl;
        std::cout << "--bind                     Address to bind to when hosting a server" << std::endl;
        std::cout << "--port               -p     Port number for the server" << std::endl;
        std::cout << "                     -o     Output path" << std::endl;
        std::cout << "--help               -h     Print help" << std::endl;
        std::cout << "--version                   Print version" << std::endl;
        std::cout << "--intro                     Run the game intro" << std::endl;
        std::cout << "--log_levels                Comma separated list of log levels, applying a minus prefix" << std::endl;
        std::cout << "                            removes the level from a group such as 'all', valid levels:" << std::endl;
        std::cout << "                            - info, warning, error, verbose, all" << std::endl;
        std::cout << "                              Example: --log_levels \"all, -verbose\", logs all but verbose levels" << std::endl;
        std::cout << "                              Default: \"info, warning, error\"" << std::endl;
        std::cout << "--all                -a     For compare, print out all divergences" << std::endl;
        std::cout << "--locomotion_path           Overrides the path to Locomotion install." << std::endl;
    }

    std::optional<int> runCommandLineOnlyCommand(const CommandLineOptions& options)
    {
        switch (options.action)
        {
            case CommandLineAction::help:
                printHelp();
                return EXIT_FAILURE;
            case CommandLineAction::version:
                printVersion();
                return EXIT_FAILURE;
            case CommandLineAction::uncompress:
                return uncompressFile(options);
            case CommandLineAction::simulate:
                return simulate(options);
            case CommandLineAction::compare:
                return compare(options);
            default:
                return std::nullopt;
        }
    }

    static int uncompressFile(const CommandLineOptions& options)
    {
        using namespace S5;

        if (options.path.empty())
        {
            Logging::error("No file specified.");
            return EXIT_FAILURE;
        }

        try
        {
            auto path = fs::u8path(options.path);

            // Copy the whole input file into memory to allow writing over input file
            MemoryStream ms;
            {
                FileStream fsInput(path, StreamMode::read);
                ms.resize(fsInput.getLength());
                fsInput.read(ms.data(), fsInput.getLength());
            }
            SawyerStreamReader reader(ms);

            auto outputPath = options.outputPath;
            if (outputPath.empty())
            {
                outputPath = options.path;
            }

            FileStream fsOutput(outputPath, StreamMode::write);
            SawyerStreamWriter writer(fsOutput);

            if (!reader.validateChecksum())
            {
                throw Exception::RuntimeError("Invalid checksum");
            }

            // Header chunk
            Header header;
            reader.readChunk(&header, sizeof(header));
            writer.writeChunk(SawyerEncoding::uncompressed, header);

            // Optional save details chunk
            if (header.hasFlags(HeaderFlags::hasSaveDetails))
            {
                auto saveDetails = std::make_unique<SaveDetails>();
                reader.readChunk(saveDetails.get(), sizeof(SaveDetails));
                writer.writeChunk(SawyerEncoding::uncompressed, *saveDetails);
            }

            // Packed objects
            for (auto i = 0; i < header.numPackedObjects; i++)
            {
                ObjectHeader object;
                reader.read(&object, sizeof(ObjectHeader));
                writer.write(&object, sizeof(ObjectHeader));

                auto chunk = reader.readChunk();
                writer.writeChunk(SawyerEncoding::uncompressed, chunk.data(), chunk.size());
            }

            if (header.type != S5Type::objects)
            {
                // Required objects chunk
                auto chunk = reader.readChunk();
                writer.writeChunk(SawyerEncoding::uncompressed, chunk.data(), chunk.size());

                // Game state chunk
                chunk = reader.readChunk();
                writer.writeChunk(SawyerEncoding::uncompressed, chunk.data(), chunk.size());

                // Tile elements chunk
                chunk = reader.readChunk();
                writer.writeChunk(SawyerEncoding::uncompressed, chunk.data(), chunk.size());
            }

            writer.writeChecksum();

            return EXIT_SUCCESS;
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to uncompress S5 file: {}", e.what());
            return EXIT_FAILURE;
        }
    }

    static int simulate(const CommandLineOptions& options)
    {
        setCommandLineOptions(options);

        if (!options.ticks)
        {
            Logging::error("Number of ticks to simulate not specified");
            return EXIT_FAILURE;
        }

        auto inPath = fs::u8path(options.path);
        auto outPath = fs::u8path(options.outputPath);
        auto comparePath = fs::u8path(options.path2);

        const auto timeStarted = std::chrono::high_resolution_clock::now();

        try
        {
            OpenLoco::simulateGame(inPath, *options.ticks);
        }
        catch (...)
        {
            Logging::error("Unable to load and simulate {}", inPath.u8string());
            return EXIT_FAILURE;
        }

        if (!options.path2.empty())
        {
            OpenLoco::GameSaveCompare::compareGameStates(comparePath);
        }

        const auto timeElapsed = std::chrono::high_resolution_clock::now() - timeStarted;

        auto& gameState = getGameState();
        Logging::info("--------------------------------");
        Logging::info("- Simulate");
        Logging::info("--------------------------------");
        Logging::info("Input:");
        Logging::info("  path: {}", inPath.u8string());
        Logging::info("  ticks: {} ticks", *options.ticks);
        Logging::info("Output:");
        Logging::info("  scenario ticks: {}", gameState.scenarioTicks);
        Logging::info("  rng:            {{ {}, {} }}", gameState.rng.srand_0(), gameState.rng.srand_1());
        Logging::info("Duration: {:%S} sec", timeElapsed);

        if (!outPath.empty())
        {
            try
            {
                S5::exportGameStateToFile(outPath, S5::SaveFlags::none);
                std::printf("  path:           %s\n", outPath.u8string().c_str());
            }
            catch (...)
            {
                Logging::error("Unable to save game to {}", outPath.u8string());
                return EXIT_FAILURE;
            }
        }

        return EXIT_SUCCESS;
    }

    static int compare(const CommandLineOptions& options)
    {
        auto file1 = fs::u8path(options.path);
        auto file2 = fs::u8path(options.path2);
        auto displayAllDivergences = options.all.empty() == false;

        if (file1.empty() || file2.empty())
        {
            Logging::error("Unable to compare gamestates...");
            Logging::error("    The required compare file paths have not been specified.");
            Logging::error("    compare [options] <path1> <path2>");
            return EXIT_FAILURE;
        }

        auto result = EXIT_SUCCESS;
        try
        {
            if (OpenLoco::GameSaveCompare::compareGameStates(file1, file2, displayAllDivergences))
            {
                Logging::info("MATCHES", file1, file2);
            }
            else
            {
                Logging::error("DOES NOT MATCH", file1, file2);
                result = EXIT_FAILURE;
            }
        }
        catch (std::exception& e)
        {
            Logging::error("Unable to compare gamestates: {} to {}", file1, file2);
            Logging::error("Exception reason {}", e.what());
            result = EXIT_FAILURE;
        }

        return result;
    }
}
