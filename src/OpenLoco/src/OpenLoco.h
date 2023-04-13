#pragma once

#include <OpenLoco/Core/FileSystem.hpp>
#include <cstdint>
#include <functional>
#include <string>

namespace OpenLoco
{
    using string_id = uint16_t;

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

    void sub_431695(uint16_t var_F253A0);
    int main(int argc, const char** argv);
    int main(const char* args);
    void promptTickLoop(std::function<bool()> tickAction);
    [[noreturn]] void exitCleanly();
    [[noreturn]] void exitWithError(OpenLoco::string_id message, uint32_t errorCode);
    [[noreturn]] void exitWithError(string_id eax, string_id ebx);
}
