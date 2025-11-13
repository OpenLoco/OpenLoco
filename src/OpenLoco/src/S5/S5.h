#pragma once

#include "Engine/Limits.h"
#include "Objects/Object.h"
#include "S5/S5GameState.h"
#include "S5/S5TileElement.h"
#include "ScenarioConstruction.h"
#include "ScenarioObjective.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/FileSystem.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace OpenLoco
{
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
        std::byte pad_C59C[0xC618 - 0xC59C]; // 0xC59C
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

    struct S5File
    {
        Header header;
        std::unique_ptr<Options> scenarioOptions;
        std::unique_ptr<SaveDetails> saveDetails;
        ObjectHeader requiredObjects[859];
        GameState gameState;
        std::vector<TileElement> tileElements;
        std::vector<std::pair<ObjectHeader, std::vector<std::byte>>> packedObjects;
    };

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

    constexpr const char* extensionSC5 = ".SC5";
    constexpr const char* extensionSV5 = ".SV5";

    constexpr const char* filterSC5 = "*.SC5";
    constexpr const char* filterSV5 = "*.SV5";

    bool exportGameStateToFile(const fs::path& path, SaveFlags flags);
    bool exportGameStateToFile(Stream& stream, SaveFlags flags);

    const std::vector<ObjectHeader>& getObjectErrorList();

    std::unique_ptr<S5File> importSave(Stream& stream);
    bool importSaveToGameState(const fs::path& path, LoadFlags flags);
    bool importSaveToGameState(Stream& stream, LoadFlags flags);
    std::unique_ptr<SaveDetails> readSaveDetails(const fs::path& path);
    std::unique_ptr<Scenario::Options> readScenarioOptions(const fs::path& path);
}
