#include "CommandLine.h"
#include "GameState.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "S5/SawyerStream.h"
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

namespace OpenLoco
{
    static CommandLineOptions _cmdlineOptions;

    static int uncompressFile(const CommandLineOptions& options);
    static int simulate(const CommandLineOptions& options);

    const CommandLineOptions& getCommandLineOptions()
    {
        return _cmdlineOptions;
    }

    void setCommandLineOptions(const CommandLineOptions& options)
    {
        _cmdlineOptions = options;
    }

    static std::vector<std::string> splitArgs(const char* args)
    {
        std::vector<std::string> arguments;
        arguments.push_back("");
        auto ch = args;
        auto arg = ch;
        auto inQuotes = false;
        while (*ch != '\0')
        {
            if (*ch == ' ' && !inQuotes)
            {
                if (ch != arg)
                {
                    arguments.emplace_back(arg, ch - arg);
                }
                arg = ch + 1;
            }
            else if (*ch == '"' && ch == arg)
            {
                inQuotes = true;
                arg = ch + 1;
            }
            else if (*ch == '"' && (*(ch + 1) == ' ' || *(ch + 1) == '\0'))
            {
                arguments.emplace_back(arg, ch - arg);
                arg = ch + 1;
                inQuotes = false;
            }
            ch++;
        }
        if (*arg != '\0')
            arguments.emplace_back(arg);
        return arguments;
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
                return true;
            return false;
        }

        static bool isShortOption(std::string_view arg)
        {
            if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-')
                return true;
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
        CommandLineParser(int argc, const char** argv)
        {
            _args.resize(argc - 1);
            for (int i = 1; i < argc; i++)
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
                return {};
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

    std::optional<CommandLineOptions> parseCommandLine(int argc, const char** argv)
    {
        auto parser = CommandLineParser(argc, argv)
                          .registerOption("-o", 1)
                          .registerOption("--help", "-h")
                          .registerOption("--version")
                          .registerOption("--intro");

        if (!parser.parse())
        {
            std::cerr << parser.getErrorMessage() << std::endl;
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
            if (firstArg == "uncompress")
            {
                options.action = CommandLineAction::uncompress;
                options.path = parser.getArg(1);
            }
            else if (firstArg == "simulate")
            {
                options.action = CommandLineAction::simulate;
                options.path = parser.getArg(1);
                options.ticks = parser.getArg<int32_t>(2);
            }
            else
            {
                options.path = parser.getArg(0);
            }
        }

        options.outputPath = parser.getArg("-o");

        return options;
    }

    std::optional<CommandLineOptions> parseCommandLine(const char* args)
    {
        auto argsV = splitArgs(args);
        std::vector<const char*> argsC;
        for (const auto& s : argsV)
        {
            argsC.push_back(s.c_str());
        }
        return parseCommandLine(argsC.size(), argsC.data());
    }

    void printVersion()
    {
        auto versionInfo = OpenLoco::getVersionInfo();
        std::cout << versionInfo << std::endl;
    }

    void printHelp()
    {
        std::cout << "usage: openloco [options] [path]" << std::endl;
        std::cout << "                uncompress [options] <path>" << std::endl;
        std::cout << "                simulate [options] <path> <ticks>" << std::endl;
        std::cout << std::endl;
        std::cout << "options:" << std::endl;
        std::cout << "           -o     Output path" << std::endl;
        std::cout << "--help     -h     Print help" << std::endl;
        std::cout << "--version         Print version" << std::endl;
        std::cout << "--intro           Run the game intro" << std::endl;
    }

    std::optional<int> runCommandLineOnlyCommand(const CommandLineOptions& options)
    {
        switch (options.action)
        {
            case CommandLineAction::help:
                printHelp();
                return 1;
            case CommandLineAction::version:
                printVersion();
                return 1;
            case CommandLineAction::uncompress:
                return uncompressFile(options);
            case CommandLineAction::simulate:
                return simulate(options);
            default:
                return {};
        }
    }

    static int uncompressFile(const CommandLineOptions& options)
    {
        using namespace S5;

        if (options.path.empty())
        {
            std::fprintf(stderr, "No file specified.\n");
            return 2;
        }

        try
        {
            auto path = fs::u8path(options.path);

            MemoryStream ms;
            SawyerStreamReader reader(path);
            SawyerStreamWriter writer(ms);

            if (!reader.validateChecksum())
            {
                throw std::runtime_error("Invalid checksum");
            }

            // Header chunk
            Header header;
            reader.readChunk(&header, sizeof(header));
            writer.writeChunk(SawyerEncoding::uncompressed, header);

            // Optional save details chunk
            if (header.flags & S5Flags::hasSaveDetails)
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
            writer.close();
            reader.close();

            auto outputPath = options.outputPath;
            if (outputPath.empty())
            {
                outputPath = options.path;
            }
            FileStream fs(outputPath, StreamFlags::write);
            fs.write(ms.data(), ms.getLength());

            return 0;
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to uncompress S5 file: %s\n", e.what());
            return 2;
        }
    }

    static int simulate(const CommandLineOptions& options)
    {
        if (!options.ticks)
        {
            std::fprintf(stderr, "Number of ticks to simulate not specified\n");
            return 2;
        }

        auto inPath = fs::u8path(options.path);
        auto outPath = fs::u8path(options.outputPath);

        try
        {
            OpenLoco::simulateGame(inPath, *options.ticks);
        }
        catch (...)
        {
            std::fprintf(stderr, "Unable to load and simulate %s\n", inPath.u8string().c_str());
        }

        auto& gameState = getGameState();
        std::printf("--------------------------------\n");
        std::printf("- Simulate\n");
        std::printf("--------------------------------\n");
        std::printf("Input:\n");
        std::printf("  path:  %s\n", inPath.u8string().c_str());
        std::printf("  ticks: %d ticks\n", *options.ticks);
        std::printf("Output:\n");
        std::printf("  scenario ticks: %u\n", gameState.scenarioTicks);
        std::printf("  rng:            { 0x%X, 0x%X }\n", gameState.rng.srand_0(), gameState.rng.srand_1());

        if (!outPath.empty())
        {
            try
            {
                S5::save(outPath, S5::SaveFlags::none);
                std::printf("  path:           %s\n", outPath.u8string().c_str());
            }
            catch (...)
            {
                std::fprintf(stderr, "Unable to save game to %s\n", outPath.u8string().c_str());
            }
        }

        return 0;
    }
}
