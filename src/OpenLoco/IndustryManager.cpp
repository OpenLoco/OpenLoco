#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Game.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "Math/Vector.hpp"
#include "Objects/BuildingObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::IndustryManager
{
    static auto& rawIndustries() { return getGameState().industries; }
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
    static uint8_t getMostCommonBuildingCargoType()
    {
        // First generate a count of all the different cargo based on what building could generate
        std::array<uint32_t, 32> cargoCounts{};
        for (size_t buildObjId = 0; buildObjId < ObjectManager::getMaxObjects(ObjectType::building); ++buildObjId)
        {
            const auto* buildObj = ObjectManager::get<BuildingObject>(buildObjId);
            if (buildObj == nullptr)
            {
                continue;
            }
            if (!(buildObj->flags & BuildingObjectFlags::misc_building)
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
            auto res = std::find(std::begin(indObj->produced_cargo_type), std::end(indObj->produced_cargo_type), cargoType);
            if (res != std::end(indObj->produced_cargo_type))
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
        if (indObj.required_cargo_type[0] == 0xFF)
        {
            return true;
        }

        if (indObj.flags & IndustryObjectFlags::requires_all_cargo)
        {
            // All required cargo must be producable in world
            for (auto i = 0; i < 3; ++i)
            {
                if (!canCargoTypeBeProducedInWorld(indObj.required_cargo_type[i]))
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
                if (canCargoTypeBeProducedInWorld(indObj.required_cargo_type[i]))
                {
                    return true;
                }
            }
            return false;
        }
    }

    // 0x00459659
    static void tryCreateNewIndustriesMonthly()
    {
        if (getFlags() & Flags::disallowIndustriesStartUp)
        {
            return;
        }

        for (size_t indObjId = 0; indObjId < ObjectManager::getMaxObjects(ObjectType::industry); ++indObjId)
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

            // 0x00459722
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
