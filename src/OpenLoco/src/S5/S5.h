#pragma once

#include "Objects/Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/FileSystem.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace OpenLoco
{
    enum class CompanyFlags : uint32_t;
    enum class GameStateFlags : uint32_t;
    class Stream;
}

namespace OpenLoco::Scenario
{
    struct Options;
}

namespace OpenLoco::S5
{
    struct Options;

    enum class S5Type : uint8_t
    {
        savedGame = 0,
        scenario = 1,
        objects = 2,
        landscape = 3,
    };

    enum class HeaderFlags : uint8_t
    {
        none = 0U,
        isRaw = 1U << 0,
        isDump = 1U << 1,
        isTitleSequence = 1U << 2,
        hasSaveDetails = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(HeaderFlags);

#pragma pack(push, 1)
    struct Header
    {
        S5Type type;
        HeaderFlags flags;
        uint16_t numPackedObjects;
        uint32_t version;
        uint32_t magic;
        std::byte padding[20];
        constexpr bool hasFlags(HeaderFlags flagsToTest) const
        {
            return (flags & flagsToTest) != HeaderFlags::none;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(Header) == 0x20);

#pragma pack(push, 1)
    struct SaveDetails
    {
        char company[256];                   // 0x000
        char owner[256];                     // 0x100
        uint32_t date;                       // 0x200
        uint16_t performanceIndex;           // 0x204 (from [company.performance_index)
        char scenario[0x40];                 // 0x206
        uint8_t challengeProgress;           // 0x246
        std::byte pad_247;                   // 0x247
        uint8_t image[250 * 200];            // 0x248
        CompanyFlags challengeFlags;         // 0xC598 (from [company.challenge_flags])
        uint16_t mapSizeX{};                 // 0xC59C // NEW
        uint16_t mapSizeY{};                 // 0xC59E // NEW
        std::byte pad_C59C[0xC618 - 0xC5A0]; // 0xC5A0
    };
#pragma pack(pop)
    static_assert(sizeof(SaveDetails) == 0xC618);

    enum class S5FixFlags : uint16_t
    {
        none = 0U,
        fixFlag0 = 1U << 0,
        fixFlag1 = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(S5FixFlags);

    struct S5File;

    enum class LoadFlags : uint32_t
    {
        none = 0U,
        titleSequence = 1U << 0,
        twoPlayer = 1U << 1,
        scenario = 1U << 2,
        landscape = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(LoadFlags);

    enum class SaveFlags : uint32_t
    {
        none = 0,
        packCustomObjects = 1U << 0,
        scenario = 1U << 1,
        landscape = 1U << 2,
        isAutosave = 1U << 28,
        noWindowClose = 1U << 29,
        raw = 1U << 30,  // Save raw data including pointers with no clean up
        dump = 1U << 31, // Used for dumping the game state when there is a fatal error
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SaveFlags);

    struct LoadError
    {
        int8_t errorCode;      // 0x0050C197
        StringId errorMessage; // 0x0050C198
        std::vector<ObjectHeader> objectList{};
    };

    constexpr uint32_t kMagicNumber = 0x62300;

    constexpr const char* extensionSC5 = ".SC5";
    constexpr const char* extensionSV5 = ".SV5";

    constexpr const char* filterSC5 = "*.SC5";
    constexpr const char* filterSV5 = "*.SV5";

    bool exportGameStateToFile(const fs::path& path, SaveFlags flags);
    bool exportGameStateToFile(Stream& stream, SaveFlags flags);

    const LoadError& getLastLoadError();
    void resetLastLoadError();

    std::unique_ptr<S5File> importSave(Stream& stream);
    bool importSaveToGameState(const fs::path& path, LoadFlags flags);
    bool importSaveToGameState(Stream& stream, LoadFlags flags);
    std::unique_ptr<SaveDetails> readSaveDetails(const fs::path& path);
    std::unique_ptr<Scenario::Options> readScenarioOptions(const fs::path& path);
}
