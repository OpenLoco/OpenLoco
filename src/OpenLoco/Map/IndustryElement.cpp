#include "IndustryElement.h"
#include "../Industry.h"
#include "../IndustryManager.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "AnimationManager.h"
#include "StationElement.h"
#include "TileLoop.hpp"
#include "TileManager.h"
#include <numeric>

namespace OpenLoco::Map
{
    Industry* IndustryElement::industry() const
    {
        return IndustryManager::get(_industryId);
    }

    uint8_t IndustryElement::buildingType() const
    {
        return (_6 >> 6) & 0x1F;
    }

    Colour IndustryElement::var_6_F800() const
    {
        return static_cast<Colour>((_6 >> 11) & 0x1F);
    }

    uint8_t IndustryElement::var_6_003F() const
    {
        return _6 & 0x3F;
    }

    void IndustryElement::setVar_6_003F(uint8_t val)
    {
        _6 &= ~(0x3F);
        _6 |= val & 0x3F;
    }

    uint8_t IndustryElement::sectionProgress() const
    {
        return (_5 >> 5) & 0x7;
    }

    void IndustryElement::setSectionProgress(uint8_t val)
    {
        _5 &= ~(0xE0);
        _5 |= (val & 0x7) << 5;
    }

    uint8_t IndustryElement::sequenceIndex() const
    {
        return _5 & 0x3;
    }

    void IndustryElement::setIsConstructed(bool val)
    {
        _type &= ~(1 << 7);
        _type |= val ? (1 << 7) : 0;
    }

    // 0x0045769A
    // A more generic version of the vanilla function
    template<typename TFunction>
    static void applyToMultiTile(IndustryElement& el0, const Map::Pos2& loc, bool isMultiTile, TFunction func)
    {
        for (auto& offset : getBuildingTileOffsets(isMultiTile))
        {
            auto* elIndustry = &el0;
            const auto pos = loc + offset.pos;
            if (offset.unk != 0)
            {
                auto tile = Map::TileManager::get(pos);
                for (auto& el : tile)
                {
                    elIndustry = el.as<IndustryElement>();
                    if (elIndustry == nullptr)
                    {
                        continue;
                    }
                    if (elIndustry->baseZ() != el0.baseZ())
                    {
                        elIndustry = nullptr;
                        continue;
                    }
                    break;
                }
            }
            if (elIndustry != nullptr)
            {
                func(*elIndustry, pos);
            }
        }
    }

    // 0x00456FF7
    bool IndustryElement::update(const Map::Pos2& loc)
    {
        // Sequence 0 updates all the other ones
        if (sequenceIndex() != 0)
        {
            return true;
        }
        if (isGhost())
        {
            return true;
        }

        auto* ind = industry();
        const auto* indObj = ind->getObject();
        const auto type = buildingType();
        const auto isMultiTile = indObj->buildingSizeFlags & (1 << type);

        if (!isConstructed())
        {
            bool newConstructed = isConstructed();
            uint8_t newSectionProgress = sectionProgress();
            uint8_t newNumSections = var_6_003F();

            const auto progress = sectionProgress();
            if (progress == 0x7)
            {
                const size_t numSections = var_6_003F();
                auto parts = indObj->getBuildingParts(type);
                if (parts.size() <= numSections + 1)
                {
                    ind->under_construction++;
                    if (ind->under_construction >= ind->numTiles)
                    {
                        ind->under_construction = 0xFF;
                        Ui::WindowManager::invalidate(Ui::WindowType::industry, enumValue(ind->id()));
                        Ui::WindowManager::invalidate(Ui::WindowType::industryList);
                    }

                    const auto height = std::accumulate(parts.begin(), parts.end(), 0, [partHeights = indObj->buildingPartHeight](int32_t total, uint8_t part) {
                        return total + partHeights[part];
                    });

                    const auto newClearZ = ((height + 3) / Map::kSmallZStep) + baseHeight();

                    for (auto& offset : getBuildingTileOffsets(isMultiTile))
                    {
                        auto* elIndustry = this;
                        const auto pos = loc + offset.pos;
                        if (offset.unk != 0)
                        {
                            auto tile = Map::TileManager::get(pos);
                            for (auto& el : tile)
                            {
                                elIndustry = el.as<IndustryElement>();
                                if (elIndustry == nullptr)
                                {
                                    continue;
                                }
                                if (elIndustry->baseZ() != baseZ())
                                {
                                    elIndustry = nullptr;
                                    continue;
                                }
                                break;
                            }
                        }
                        if (elIndustry != nullptr)
                        {
                            Ui::ViewportManager::invalidate(pos, elIndustry->baseHeight(), elIndustry->clearHeight(), ZoomLevel::quarter);
                            elIndustry->setClearZ(newClearZ);
                        }
                    }
                    newConstructed = true;
                    newSectionProgress = 0;
                    newNumSections = 0;
                }
                else
                {
                    newSectionProgress = 0;
                    newNumSections++;
                }
            }
            else
            {
                newSectionProgress++;
            }

            for (auto& offset : getBuildingTileOffsets(isMultiTile))
            {
                auto* elIndustry = this;
                const auto pos = loc + offset.pos;
                if (offset.unk != 0)
                {
                    auto tile = Map::TileManager::get(pos);
                    for (auto& el : tile)
                    {
                        elIndustry = el.as<IndustryElement>();
                        if (elIndustry == nullptr)
                        {
                            continue;
                        }
                        if (elIndustry->baseZ() != baseZ())
                        {
                            elIndustry = nullptr;
                            continue;
                        }
                        break;
                    }
                }
                if (elIndustry != nullptr)
                {
                    elIndustry->setIsConstructed(newConstructed);
                    elIndustry->setSectionProgress(newSectionProgress);
                    elIndustry->setVar_6_003F(newNumSections);
                    Ui::ViewportManager::invalidate(pos, elIndustry->baseHeight(), elIndustry->clearHeight(), ZoomLevel::quarter);
                }
            }
        }

        // Might have changed so can't be combined with above if
        if (isConstructed())
        {
            bool hasAZeroFrame = false;
            for (auto& part : indObj->getBuildingParts(type))
            {
                const auto animFrames = indObj->buildingPartAnimations[part].numFrames;
                if (animFrames == 0)
                {
                    hasAZeroFrame = true;
                    break;
                }
            }
            if (hasAZeroFrame && !(var_6_003F() & (1 << 5)))
            {
                std::array<uint8_t, 8> _E0C3D4{};
                auto ptr = _E0C3D4.begin();
                for (const auto& unk38 : indObj->getUnk38())
                {
                    if (unk38.var_00 == type)
                    {
                        *ptr++ = unk38.var_01;
                    }
                }
                const auto numAnimations = std::distance(_E0C3D4.begin(), ptr);
                if (numAnimations != 0)
                {
                    const auto rand = ind->prng.randNext();
                    if ((rand & 0x7) == 0)
                    {
                        const auto randAnim = _E0C3D4[(numAnimations * (rand & 0xFF)) / 256];
                        applyToMultiTile(*this, loc, isMultiTile, [var_6 = randAnim | (1 << 5)](IndustryElement& elIndustry, const Map::Pos2& pos) {
                            elIndustry.setVar_6_003F(var_6);
                        });
                        AnimationManager::createAnimation(4, loc, baseZ());
                    }
                }
            }
        }

        if (ind->under_construction == 0xFF)
        {
            const coord_t upperRange = isMultiTile ? 10 : 8;
            constexpr coord_t kLowerRange = 8;

            // Find all stations in range of industry building
            for (auto& tilePos : TilePosRangeView(Map::TilePos2(loc) - Map::TilePos2{ kLowerRange, kLowerRange }, Map::TilePos2(loc) + Map::TilePos2{ upperRange, upperRange }))
            {
                if (!validCoords(tilePos))
                {
                    continue;
                }

                auto tile = TileManager::get(tilePos);
                for (auto& el : tile)
                {
                    auto* elStation = el.as<StationElement>();
                    if (elStation == nullptr)
                    {
                        continue;
                    }
                    if (elStation->isGhost() || elStation->isFlag5())
                    {
                        continue;
                    }
                    const auto station = elStation->stationId();
                    ind->var_E1.set(enumValue(station), true);
                }
            }
        }
        return true;
    }
}
