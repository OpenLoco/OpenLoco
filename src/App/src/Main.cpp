#include <OpenLoco/Audio/Audio.h>
#include <OpenLoco/CommandLine.h>
#include <OpenLoco/Config.h>
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Core/FileStream.h>
#include <OpenLoco/Core/MemoryStream.h>
#include <OpenLoco/Environment.h>
#include <OpenLoco/GameSaveCompare.h>
#include <OpenLoco/GameState.h>
#include <OpenLoco/Intro.h>
#include <OpenLoco/Logging.h>
#include <OpenLoco/Objects/Object.h>
#include <OpenLoco/OpenLoco.h>
#include <OpenLoco/Platform/Crash.h>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/S5/S5.h>
#include <OpenLoco/S5/SawyerStream.h>
#include <OpenLoco/Title.h>
#include <OpenLoco/Version.hpp>
#include <SDL3/SDL_main.h>
#include <iostream>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco
{

    static void printVersion()
    {
        auto versionInfo = Version::getVersionInfo();
        std::cout << versionInfo << std::endl;
    }

    static void printHelp()
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

    // 0x00406386
    static void run()
    {
        auto& cfg = Config::get();

        initialise();

        Ui::createWindow(cfg.display);
        Audio::initialiseDSound();

        const auto& cmdLineOptions = getCommandLineOptions();
        if (cmdLineOptions.action == CommandLineAction::intro)
        {
            Intro::state(Intro::State::begin);
        }
        else
        {
            Intro::state(Intro::State::end);
        }

        Title::start();

        while (Input::processMessages())
        {
            update();
        }
    }

    static std::optional<int> runCommandLineOnlyCommand(const CommandLineOptions& options)
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

    // 0x00406D13
    static int main(CommandLineOptions&& options)
    {
        // Bootstrap the logging system.
        Logging::initialize(options.logLevels);

        // Always print the product name, version, and platform info first.
        Logging::info("{}", Version::getVersionInfo());
        Logging::info("{}", Version::getPlatformInfo());

        setCommandLineOptions(options);

        Config::read();
        if (options.locomotionDataPath.has_value())
        {
            Config::get().locoInstallPath = options.locomotionDataPath.value();
        }
        Environment::setLocale();
        Environment::resolvePaths();

        auto ret = runCommandLineOnlyCommand(options);
        if (ret)
        {
            return *ret;
        }

        if (!OpenLoco::Platform::isRunningInWine())
        {
            CrashHandler::AppInfo appInfo;
            appInfo.name = "OpenLoco";
            appInfo.version = Version::getVersionInfo();

            CrashHandler::init(appInfo);
        }
        else
        {
            Logging::warn("Detected wine, not installing crash handler as it doesn't provide useful data. Consider using native builds of OpenLoco instead.");
        }

        try
        {
            run();
            exitCleanly();
        }
        catch (const std::exception& e)
        {
            Logging::error("Exception: {}", e.what());
            Ui::showMessageBox("Exception", e.what());
            exitCleanly();
        }
        catch (...)
        {
            Ui::showMessageBox("Exception", "Unsure what threw the exception!");
            exitCleanly();
        }
    }

}

int main(int argc, const char** argv)
{
    OpenLoco::Platform::initialise();

    auto options = OpenLoco::parseCommandLine(OpenLoco::Platform::getCmdLineVector(argc, argv));
    if (!options)
    {
        OpenLoco::printHelp();
        return EXIT_FAILURE;
    }

    SDL_SetMainReady();

    return OpenLoco::main(std::move(*options));
}
