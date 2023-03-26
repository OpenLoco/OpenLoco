#include "Logging.h"

#include <OpenLoco/Diagnostics/LogFile.h>
#include <OpenLoco/Diagnostics/LogTerminal.h>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Platform/Platform.h>
#include <fmt/chrono.h>

namespace OpenLoco::Diagnostics::Logging
{
    static std::shared_ptr<LogTerminal> _terminalLogSink{};
    static std::shared_ptr<LogFile> _fileLogSink{};

    // The maximum amount of log files to keep in the folder, will delete the oldest
    // logs if the amount of file exceeds this number.
    static constexpr auto kMaxLogFiles = 20;

    static fs::path getLogsFolderPath()
    {
        return Platform::getUserDirectory() / "logs";
    }

    static std::string getLogFileName()
    {
        return fmt::format("openloco_{:%Y-%m-%d_%H_%M_%S}.log", fmt::localtime(std::time(nullptr)));
    }

    static void cleanupLogFiles(const fs::path& logsFolderPath)
    {
        // Get all log files in the folder
        std::vector<std::filesystem::path> logFiles;
        for (const auto& file : std::filesystem::directory_iterator(logsFolderPath))
        {
            if (file.path().extension() == ".log")
            {
                logFiles.push_back(file.path());
            }
        }

        // Sort log files by last write time in ascending order
        std::sort(logFiles.begin(), logFiles.end(), [](const auto& a, const auto& b) {
            return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
        });

        // Delete oldest log files
        if (logFiles.size() > kMaxLogFiles)
        {
            int numFilesToDelete = logFiles.size() - kMaxLogFiles;
            for (int i = 0; i < numFilesToDelete; i++)
            {
                std::filesystem::remove(logFiles[i]);
            }
        }
    }

    static uint32_t parseLogLevels(std::string_view logLevels)
    {
        uint32_t res{};

        auto addLogLevel = [&res](std::string_view level) {
            if (level == "info")
                res |= 1U << static_cast<unsigned>(Logging::Level::info);
            else if (level == "warning")
                res |= 1U << static_cast<unsigned>(Logging::Level::warning);
            else if (level == "error")
                res |= 1U << static_cast<unsigned>(Logging::Level::error);
            else if (level == "verbose")
                res |= 1U << static_cast<unsigned>(Logging::Level::verbose);
            else if (level == "all")
                res = ~0U;
        };

        constexpr char separator = ',';

        size_t start = 0;
        size_t end = logLevels.find(separator);

        while (end != std::string_view::npos)
        {
            std::string_view substring = logLevels.substr(start, end - start);
            substring = substring.substr(0, substring.find_last_not_of(" \t\n\r") + 1);
            addLogLevel(substring);
            start = end + 1;
            end = logLevels.find(separator, start);
        }

        std::string_view substring = logLevels.substr(start);
        substring = substring.substr(0, substring.find_last_not_of(" \t\n\r") + 1);
        addLogLevel(substring);

        return res;
    }

    void initialize(std::string_view logLevels)
    {
        const auto logsFolder = getLogsFolderPath();
        cleanupLogFiles(logsFolder);

        const auto logLevelMask = parseLogLevels(logLevels);

        // Setup sink for the terminal/console.
        _terminalLogSink = std::make_shared<LogTerminal>();
        _terminalLogSink->setWriteTimestamps(false);
        _terminalLogSink->setLevelMask(logLevelMask);
        Logging::installSink(_terminalLogSink);

        // Setup log file sink.
        const auto logFile = logsFolder / getLogFileName();
        _fileLogSink = std::make_shared<LogFile>(logFile);
        _fileLogSink->setWriteTimestamps(true);
        _fileLogSink->setLevelMask(logLevelMask);
        Logging::installSink(_fileLogSink);
    }

    void shutdown()
    {
        Logging::removeSink(_fileLogSink);
        Logging::removeSink(_terminalLogSink);
    }
}
