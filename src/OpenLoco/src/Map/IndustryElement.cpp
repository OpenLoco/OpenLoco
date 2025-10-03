#include "IndustryElement.h"
#include "Animation.h"
#include "AnimationManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "ScenarioManager.h"
#include "StationElement.h"
#include "TileLoop.hpp"
#include "TileManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/Industry.h"
#include "World/IndustryManager.h"
#include <numeric>

namespace OpenLoco::World
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
    static void applyToMultiTile(IndustryElement& el0, const World::Pos2& loc, bool isMultiTile, TFunction&& func)
    {
        for (auto& offset : getBuildingTileOffsets(isMultiTile))
        {
            auto* elIndustry = &el0;
            const auto pos = loc + offset.pos;
            if (offset.index != 0)
            {
                auto tile = World::TileManager::get(pos);
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
    bool IndustryElement::update(const World::Pos2& loc)
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
                const auto parts = indObj->getBuildingParts(type);
                const auto heights = indObj->getBuildingPartHeights();
                if (parts.size() <= numSections + 1)
                {
                    ind->under_construction++;
                    if (ind->under_construction >= ind->numTiles)
                    {
                        ind->under_construction = 0xFF;
                        Ui::WindowManager::invalidate(Ui::WindowType::industry, enumValue(ind->id()));
                        Ui::WindowManager::invalidate(Ui::WindowType::industryList);
                    }

                    const auto height = std::accumulate(parts.begin(), parts.end(), 0, [partHeights = heights](int32_t total, uint8_t part) {
                        return total + partHeights[part];
                    });

                    const auto newClearZ = ((height + 3) / World::kSmallZStep) + baseZ();

                    applyToMultiTile(*this, loc, isMultiTile, [newClearZ](World::IndustryElement& elIndustry, const World::Pos2& pos) {
                        Ui::ViewportManager::invalidate(pos, elIndustry.baseHeight(), elIndustry.clearHeight(), ZoomLevel::quarter);
                        elIndustry.setClearZ(newClearZ);
                    });

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

            applyToMultiTile(*this, loc, isMultiTile, [newConstructed, newSectionProgress, newNumSections](World::IndustryElement& elIndustry, const World::Pos2& pos) {
                elIndustry.setIsConstructed(newConstructed);
                elIndustry.setSectionProgress(newSectionProgress);
                elIndustry.setVar_6_003F(newNumSections);
                Ui::ViewportManager::invalidate(pos, elIndustry.baseHeight(), elIndustry.clearHeight(), ZoomLevel::quarter);
            });
        }

        // Might have changed so can't be combined with above if
        if (isConstructed())
        {
            bool hasAZeroFrame = false;
            const auto buildingPartAnims = indObj->getBuildingPartAnimations();
            for (auto& part : indObj->getBuildingParts(type))
            {
                const auto animFrames = buildingPartAnims[part].numFrames;
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
                    if ((rand & 0x700) == 0)
                    {
                        const auto randAnim = _E0C3D4[(numAnimations * (rand & 0xFF)) / 256];
                        const auto newVar6_3F = randAnim | (1 << 5) | (var_6_003F() & 0xC);
                        applyToMultiTile(*this, loc, isMultiTile, [newVar6_3F](IndustryElement& elIndustry, [[maybe_unused]] const World::Pos2& pos) {
                            elIndustry.setVar_6_003F(newVar6_3F);
                        });
                        AnimationManager::createAnimation(4, loc, baseZ());
                    }
                }
            }
        }

        if (ind->under_construction == 0xFF)
        {
            const coord_t upperRange = isMultiTile ? 5 : 4;
            constexpr coord_t kLowerRange = 4;

            // Find all stations in range of industry building
            const auto tileStart = World::toTileSpace(loc);
            for (auto& tilePos : getClampedRange(tileStart - TilePos2{ kLowerRange, kLowerRange }, tileStart + TilePos2{ upperRange, upperRange }))
            {
                auto tile = TileManager::get(tilePos);
                for (auto& el : tile)
                {
                    auto* elStation = el.as<StationElement>();
                    if (elStation == nullptr)
                    {
                        continue;
                    }
                    if (elStation->isGhost() || elStation->isAiAllocated())
                    {
                        continue;
                    }
                    const auto station = elStation->stationId();
                    ind->stationsInRange.set(enumValue(station), true);
                }
            }
        }
        return true;
    }

    // 0x00456E32
    bool updateIndustryAnimation1(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
        for (auto& el : tile)
        {
            auto* elIndustry = el.as<IndustryElement>();
            if (elIndustry == nullptr)
            {
                continue;
            }
            if (elIndustry->baseZ() != anim.baseZ)
            {
                continue;
            }

            auto* industry = elIndustry->industry();
            const auto* indObj = industry->getObject();
            const auto buildingParts = indObj->getBuildingParts(elIndustry->buildingType());
            const auto buildingPartAnims = indObj->getBuildingPartAnimations();
            bool hasAnimation = false;
            uint8_t animSpeed = std::numeric_limits<uint8_t>::max();
            for (auto& part : buildingParts)
            {
                auto& partAnim = buildingPartAnims[part];
                if (partAnim.numFrames > 1)
                {
                    hasAnimation = true;
                    animSpeed = std::min<uint8_t>(animSpeed, partAnim.animationSpeed & ~(1 << 7));
                }
            }
            if (!hasAnimation)
            {
                return true;
            }
            const auto speedMask = ((1 << animSpeed) - 1);
            if (!(ScenarioManager::getScenarioTicks() & speedMask))
            {
                Ui::ViewportManager::invalidate(anim.pos, el.baseHeight(), el.clearHeight(), ZoomLevel::quarter);
            }
            return false;
        }
        return true;
    }

    // 0x00456EEB
    bool updateIndustryAnimation2(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
        for (auto& el : tile)
        {
            auto* elIndustry = el.as<IndustryElement>();
            if (elIndustry == nullptr)
            {
                continue;
            }
            if (elIndustry->baseZ() != anim.baseZ)
            {
                continue;
            }
            if (!(elIndustry->var_6_003F() & (1 << 5)))
            {
                continue;
            }

            auto* industry = elIndustry->industry();
            const auto* indObj = industry->getObject();
            const auto type = elIndustry->buildingType();
            const auto buildingParts = indObj->getBuildingParts(type);
            const auto buildingPartAnims = indObj->getBuildingPartAnimations();
            // Guaranteed power of 2
            auto animLength = indObj->getAnimationSequence(elIndustry->var_6_003F() & 0x3).size();
            const auto isMultiTile = indObj->buildingSizeFlags & (1 << type);

            for (auto& part : buildingParts)
            {
                auto& partAnim = buildingPartAnims[part];
                if (partAnim.numFrames == 0)
                {
                    const auto animSpeed = partAnim.animationSpeed & ~(1 << 7);
                    const auto speedMask = animLength - 1;
                    if (elIndustry->var_6_003F() & (1 << 4))
                    {
                        if ((speedMask & (ScenarioManager::getScenarioTicks() >> animSpeed)) == 0)
                        {
                            applyToMultiTile(*elIndustry, anim.pos, isMultiTile, [](World::IndustryElement& elIndustry, const World::Pos2& pos) {
                                Ui::ViewportManager::invalidate(pos, elIndustry.baseHeight(), elIndustry.clearHeight(), ZoomLevel::quarter);
                                elIndustry.setVar_6_003F(elIndustry.var_6_003F() & ~(1 << 5));
                            });
                            return true;
                        }
                        else
                        {
                            const auto speedMask2 = (1 << animSpeed) - 1;
                            if (!(speedMask2 & ScenarioManager::getScenarioTicks()))
                            {
                                applyToMultiTile(*elIndustry, anim.pos, isMultiTile, [](World::IndustryElement& elIndustry, const World::Pos2& pos) {
                                    Ui::ViewportManager::invalidate(pos, elIndustry.baseHeight(), elIndustry.clearHeight(), ZoomLevel::quarter);
                                });
                            }
                            return false;
                        }
                    }
                    else
                    {
                        if ((speedMask & (ScenarioManager::getScenarioTicks() >> animSpeed)) == 1)
                        {
                            applyToMultiTile(*elIndustry, anim.pos, isMultiTile, [](World::IndustryElement& elIndustry, const World::Pos2& pos) {
                                Ui::ViewportManager::invalidate(pos, elIndustry.baseHeight(), elIndustry.clearHeight(), ZoomLevel::quarter);
                                elIndustry.setVar_6_003F(elIndustry.var_6_003F() | (1 << 4));
                            });
                        }
                        return false;
                    }
                }
            }
        }
        return true;
    }
}
