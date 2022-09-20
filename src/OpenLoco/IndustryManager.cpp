#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "Math/Vector.hpp"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <numeric>

using namespace OpenLoco::Interop;

namespace OpenLoco::IndustryManager
{
    static auto& rawIndustries() { return getGameState().industries; }
    static auto getTotalIndustriesCap() { return getGameState().numberOfIndustries; }
    uint8_t getFlags() { return getGameState().industryFlags; }

    void setFlags(const uint8_t flags)
    {
        getGameState().industryFlags = flags;
    }

    // 0x00453214
    void reset()
    {
        for (auto& industry : rawIndustries())
        {
            industry.name = StringIds::null;
        }
        Ui::Windows::IndustryList::reset();
    }

    FixedVector<Industry, Limits::kMaxIndustries> industries()
    {
        return FixedVector(rawIndustries());
    }

    Industry* get(IndustryId id)
    {
        if (enumValue(id) >= Limits::kMaxIndustries)
        {
            return nullptr;
        }
        return &rawIndustries()[enumValue(id)];
    }

    // 0x00453234
    void update()
    {
        if (Game::hasFlags(1u << 0) && !isEditorMode())
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.update();
            }
        }
    }

    // 0x00453487
    void updateDaily()
    {
        if (Game::hasFlags(1u << 0))
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.updateDaily();
            }
        }
    }

    // 0x0047EA42
    static size_t getMostCommonBuildingCargoType()
    {
        // First generate a count of all the different cargo based on what building could generate
        std::array<uint32_t, ObjectManager::getMaxObjects(ObjectType::cargo)> cargoCounts{};
        for (size_t buildObjId = 0; buildObjId < ObjectManager::getMaxObjects(ObjectType::building); ++buildObjId)
        {
            const auto* buildObj = ObjectManager::get<BuildingObject>(buildObjId);
            if (buildObj == nullptr)
            {
                continue;
            }
            if (!(buildObj->flags & BuildingObjectFlags::miscBuilding)
                && buildObj->producedQuantity[0] != 0)
            {
                cargoCounts[buildObj->producedCargoType[0]]++;
            }
        }
        // Then pick the most common
        auto maxEl = std::max_element(std::begin(cargoCounts), std::end(cargoCounts));
        if (*maxEl != 0)
        {
            return std::distance(std::begin(cargoCounts), maxEl);
        }

        // If none are common pick any valid cargo object
        for (size_t cargoObjId = 0; cargoObjId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoObjId)
        {
            const auto* cargoObj = ObjectManager::get<CargoObject>(cargoObjId);
            if (cargoObj == nullptr)
            {
                continue;
            }
            return cargoObjId;
        }

        // This should really be an error!
        return 0;
    }

    // 0x0045960E
    static bool canCargoTypeBeProducedInWorld(uint8_t cargoType)
    {
        if (cargoType == getMostCommonBuildingCargoType())
        {
            return true;
        }

        for (auto& industry : industries())
        {
            const auto* indObj = industry.getObject();
            auto res = std::find(std::begin(indObj->producedCargoType), std::end(indObj->producedCargoType), cargoType);
            if (res != std::end(indObj->producedCargoType))
            {
                return true;
            }
        }
        return false;
    }

    static bool canIndustryObjBeCreated(const IndustryObject& indObj)
    {
        if (getCurrentYear() < indObj.designedYear)
        {
            return false;
        }

        if (getCurrentYear() >= indObj.obsoleteYear)
        {
            return false;
        }

        // If industry is a self producer i.e. no requirements to produce
        if (indObj.requiredCargoType[0] == 0xFF)
        {
            return true;
        }

        if (indObj.flags & IndustryObjectFlags::requiresAllCargo)
        {
            // All required cargo must be producable in world
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj.requiredCargoType[i] == 0xFF)
                {
                    continue;
                }
                if (!canCargoTypeBeProducedInWorld(indObj.requiredCargoType[i]))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            // At least one required cargo must be producable in world
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj.requiredCargoType[i] == 0xFF)
                {
                    continue;
                }
                if (canCargoTypeBeProducedInWorld(indObj.requiredCargoType[i]))
                {
                    return true;
                }
            }
            return false;
        }
    }

    // 0x004599B3
    static std::optional<Map::Pos2> findRandomNewIndustryLocation(const uint8_t indObjId)
    {
        registers regs;
        regs.edx = indObjId;
        call(0x004599B3, regs);
        if (regs.ax == -1)
        {
            return std::nullopt;
        }

        return Map::Pos2{ regs.ax, regs.cx };
    }

    // 0x00459722 & 0x004598F0
    static int32_t capOfTypeOfIndustry(const uint8_t indObjId)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(indObjId);

        // totalOfTypeInScenario is in the range of 1->32 inclusive
        // getTotalIndustriesCap() is in the range of 0->2 (low, med, high)
        // This formula is ultimately staticPreferredTotalOfType = 1/4 * ((getTotalIndustriesCap() + 1) * indObj->totalOfTypeInScenario)
        // but we will do it in two steps to keep identical results.
        const auto intermediate1 = ((getTotalIndustriesCap() + 1) * indObj->totalOfTypeInScenario) / 3;
        const auto staticPreferredTotalOfType = intermediate1 - intermediate1 / 4;
        // The preferred total can vary by up to a half of the static preffered total.
        const auto randomPreferredTotalOfType = (staticPreferredTotalOfType / 2) * gPrng().randNext(0xFF) / 256;
        return staticPreferredTotalOfType + randomPreferredTotalOfType;
    }

    // 0x00459722
    static bool hasReachedCapOfTypeOfIndustry(const uint8_t indObjId)
    {
        const auto preferredTotalOfType = capOfTypeOfIndustry(indObjId);

        const auto totalOfThisType = std::count_if(std::begin(industries()), std::end(industries()), [indObjId](const auto& industry) {
            return (industry.objectId == indObjId);
        });

        return totalOfThisType >= preferredTotalOfType;
    }

    // 0x0045979C & 0x00459949
    static void createNewIndustry(const uint8_t indObjId, const bool buildImmediately, const int32_t numAttempts)
    {
        // Try find valid coordinates for this industry
        for (auto attempt = 0; attempt < numAttempts; ++attempt)
        {
            auto randomIndustryLoc = findRandomNewIndustryLocation(indObjId);
            if (randomIndustryLoc.has_value())
            {
                GameCommands::IndustryPlacementArgs args;
                args.type = indObjId;
                args.buildImmediately = buildImmediately;
                args.pos = *randomIndustryLoc;
                gPrng().randNext();
                args.srand0 = gPrng().srand_0();
                args.srand1 = gPrng().srand_1();

                auto res = GameCommands::doCommand(args, GameCommands::Flags::apply);
                if (res != GameCommands::FAILURE)
                {
                    break;
                }
            }
        }
    }

    // 0x00459659
    static void tryCreateNewIndustriesMonthly()
    {
        if (getFlags() & Flags::disallowIndustriesStartUp)
        {
            return;
        }

        for (uint8_t indObjId = 0; static_cast<size_t>(indObjId) < ObjectManager::getMaxObjects(ObjectType::industry); ++indObjId)
        {
            auto* indObj = ObjectManager::get<IndustryObject>(indObjId);
            if (indObj == nullptr)
            {
                continue;
            }

            if (!canIndustryObjBeCreated(*indObj))
            {
                continue;
            }

            if (hasReachedCapOfTypeOfIndustry(indObjId))
            {
                continue;
            }
            createNewIndustry(indObjId, false, 25);
        }
    }

    // 0x0045383B
    void updateMonthly()
    {
        if (Game::hasFlags(1u << 0))
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            tryCreateNewIndustriesMonthly();

            for (auto& industry : industries())
            {
                industry.updateMonthly();
            }

            Ui::WindowManager::invalidate(Ui::WindowType::industry);
        }
    }

    // 0x00459D2D
    void createAllMapAnimations()
    {
        if (!Game::hasFlags(1u << 0))
            return;

        for (auto& industry : industries())
        {
            industry.createMapAnimations();
        }
    }

    // 0x048FE92
    bool industryNearPosition(const Map::Pos2& position, uint32_t flags)
    {
        for (auto& industry : industries())
        {
            const auto* industryObj = industry.getObject();
            if ((industryObj->flags & flags) == 0)
                continue;

            auto manhattanDistance = Math::Vector::manhattanDistance(Map::Pos2{ industry.x, industry.y }, position);
            if (manhattanDistance / Map::kTileSize < 11)
                return true;
        }

        return false;
    }

    // 0x004574E8
    void updateProducedCargoStats()
    {
        for (auto& industry : industries())
        {
            industry.updateProducedCargoStats();
        }
    }
}

OpenLoco::IndustryId OpenLoco::Industry::id() const
{
    auto* first = &IndustryManager::rawIndustries()[0];
    return IndustryId(this - first);
}
