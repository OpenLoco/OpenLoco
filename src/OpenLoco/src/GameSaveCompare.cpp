#include "GameSaveCompare.h"

#include <algorithm>
#include <string>

#include "GameState.h"
#include "Logging.h"
#include "OpenLoco.h"
#include "S5/Limits.h"
#include "S5/S5.h"
#include "S5/S5File.h"
#include "S5/S5Options.h"
#include <OpenLoco/Core/FileStream.h>
#include <OpenLoco/Core/MemoryStream.h>
#include <OpenLoco/Reflection/S5/GameState.h>
#include <type_traits>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::GameSaveCompare
{
    template<typename T>
    static bool compareField(const std::string& path, const T& lhs, const T& rhs)
    {
        if constexpr (requires { typename Reflection<T>::Fields; })
        {
            bool foundDivergence = false;
            [&]<typename... Fields>(FieldList<Fields...>) {
                ((foundDivergence |= compareField(path + "." + std::string(Fields::name), lhs.*Fields::field, rhs.*Fields::field)), ...);
            }(typename Reflection<T>::Fields{});
            return foundDivergence;
        }
        else if constexpr (std::is_array_v<T>)
        {
            bool foundDivergence = false;
            for (std::size_t i = 0; i < std::extent_v<T>; ++i)
            {
                foundDivergence |= compareField(path + "[" + std::to_string(i) + "]", lhs[i], rhs[i]);
            }
            return foundDivergence;
        }
        else
        {
            if (lhs == rhs)
            {
                return false;
            }
            Logging::info("DIVERGENCE");
            Logging::info("TYPE: {}", path);
            if constexpr (std::is_enum_v<T>)
            {
                Logging::info("    LHS: {}", static_cast<std::underlying_type_t<T>>(lhs));
                Logging::info("    RHS: {}", static_cast<std::underlying_type_t<T>>(rhs));
            }
            else
            {
                Logging::info("    LHS: {}", lhs);
                Logging::info("    RHS: {}", rhs);
            }
            return true;
        }
    }

    static bool compareGameStates(S5::GameState& gameState1, S5::GameState& gameState2, bool displayAllDivergences)
    {
        if (displayAllDivergences)
        {
            Logging::info("display all divergences!");
        }

        return not compareField("gameState", gameState1, gameState2);
    }

    static bool compareTileElement(const std::string& path, const S5::TileElement& lhs, const S5::TileElement& rhs, bool displayAllDivergences)
    {
        const auto lhsBytes = lhs.rawData();
        const auto rhsBytes = rhs.rawData();
        if (std::ranges::equal(lhsBytes, rhsBytes))
        {
            return false;
        }

        Logging::info("DIVERGENCE");
        Logging::info("TYPE: {} type[{}]", path, static_cast<int>(enumValue(lhs.type())));
        if (displayAllDivergences)
        {
            for (size_t offset = 0; offset < lhsBytes.size(); ++offset)
            {
                if (lhsBytes[offset] != rhsBytes[offset])
                {
                    Logging::info("    OFFSET: {}", offset);
                    Logging::info("    LHS: {:#x}", lhsBytes[offset]);
                    Logging::info("    RHS: {:#x}", rhsBytes[offset]);
                }
            }
        }
        return true;
    }

    bool compareElements(const std::vector<S5::TileElement>& tileElements1, const std::vector<S5::TileElement>& tileElements2, bool displayAllDivergences)
    {
        if (tileElements1.size() != tileElements2.size())
        {
            Logging::info("The TileElements sizes are different.");
            Logging::info("Size of TileElements1 = {}", tileElements1.size());
            Logging::info("Size of TileElements2 = {}", tileElements2.size());
        }

        bool foundDivergence = false;
        auto iterator1 = tileElements1.begin();
        auto iterator2 = tileElements2.begin();
        for (auto y = 0; y < 384; ++y)
        {
            for (auto x = 0; x < 384; ++x)
            {
                auto allElementsOnTile = [](auto& iter) {
                    std::vector<S5::TileElement> ts;
                    do
                    {
                        ts.push_back(*iter);
                    } while (!iter++->isLast());
                    return ts;
                };
                const auto t1s = allElementsOnTile(iterator1);
                const auto t2s = allElementsOnTile(iterator2);

                const auto shared = std::min(t1s.size(), t2s.size());
                for (size_t i = 0; i < shared; ++i)
                {
                    const auto path = "tile[" + std::to_string(x) + "," + std::to_string(y) + "].element[" + std::to_string(i) + "]";
                    foundDivergence |= compareTileElement(path, t1s[i], t2s[i], displayAllDivergences);
                }
                for (size_t i = shared; i < t1s.size(); ++i)
                {
                    Logging::info("DIVERGENCE");
                    Logging::info("Extra tile[{},{}] element[{}]", x, y, i);
                    foundDivergence = true;
                }
                for (size_t i = shared; i < t2s.size(); ++i)
                {
                    Logging::info("DIVERGENCE");
                    Logging::info("Removed tile[{},{}] element[{}]", x, y, i);
                    foundDivergence = true;
                }
            }
        }

        return not foundDivergence;
    }

    bool compareGameStates(const fs::path& path)
    {
        Logging::info("Comparing reference file {} to current GameState frame", path);

        MemoryStream ms;
        if (!S5::exportGameStateToFile(ms, S5::SaveFlags::noWindowClose))
        {
            Logging::error("Failed to export current game state for comparison");
            return false;
        }

        ms.setPosition(0);
        auto currentGameState = S5::loadSave(ms);
        if (currentGameState == nullptr)
        {
            Logging::error("Failed to reload exported current game state for comparison");
            return false;
        }

        FileStream referenceFile(path, StreamMode::read);
        auto referenceGameState = S5::loadSave(referenceFile);
        return compareGameStates(currentGameState->gameState, referenceGameState->gameState, false);
    }

    bool compareGameStates(const fs::path& path1, const fs::path& path2, bool displayAllDivergences)
    {
        Logging::info("Comparing game state files:");
        Logging::info("   file1: {}", path1);
        Logging::info("   file2: {}", path2);

        FileStream file1(path1, StreamMode::read);
        auto state1 = S5::loadSave(file1);
        FileStream file2(path2, StreamMode::read);
        auto state2 = S5::loadSave(file2);
        auto match = compareGameStates(state1->gameState, state2->gameState, displayAllDivergences);
        match &= compareElements(state1->tileElements, state2->tileElements, displayAllDivergences);
        return match;
    }
}
