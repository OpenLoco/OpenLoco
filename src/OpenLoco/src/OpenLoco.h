#pragma once

#include <OpenLoco/Core/FileSystem.hpp>
#include <cstdint>
#include <functional>
#include <string>

namespace OpenLoco
{
    using StringId = uint16_t;

    namespace Engine
    {
        constexpr uint32_t MaxTimeDeltaMs = 500;
        constexpr uint32_t UpdateRateHz = 40;
        constexpr uint32_t UpdateRateInMs = 1000 / UpdateRateHz;
        constexpr uint32_t MaxUpdates = 3;
    }

    extern const char version[];
    std::string getVersionInfo();

    void* hInstance();
    void initialiseViewports();
    void simulateGame(const fs::path& path, int32_t ticks);
    void compareGameStates(const fs::path& path);
    void compareGameStates(const fs::path& path1, const fs::path& path2);

    void sub_431695(uint16_t var_F253A0);
    int main(std::vector<std::string>&& argv);
    bool promptTickLoop(std::function<bool()> tickAction);
    [[noreturn]] void exitCleanly();
    [[noreturn]] void exitWithError(OpenLoco::StringId message, uint32_t errorCode);
    [[noreturn]] void exitWithError(StringId eax, StringId ebx);
}
