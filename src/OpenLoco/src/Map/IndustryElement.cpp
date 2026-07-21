#include "Map/IndustryElement.h"
#include "Map/Animation.h"
#include "Map/AnimationManager.h"
#include "Map/StationElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Scenario/ScenarioManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/Industry.h"
#include "World/IndustryManager.h"
#include <numeric>

namespace OpenLoco::World
{
    constexpr auto kSectionComplete = 0x7;
    constexpr auto kConstructionComplete = std::numeric_limits<uint8_t>::max();

    Industry* IndustryElement::industry() const
    {
        return IndustryManager::get(_industryId);
    }

    uint8_t IndustryElement::buildingType() const
    {
        return (_6 & kIndustryElement6BuildingTypeMask) >> 6;
    }

    Colour IndustryElement::colour() const
    {
        return static_cast<Colour>((_6 & kIndustryElement6ColourMask) >> 11);
    }

    uint8_t IndustryElement::sectionsCompleted() const
    {
        return _6 & kIndustryElement6SectionsCompletedMask;
    }

    void IndustryElement::setSectionsCompleted(uint8_t val)
    {
        _6 &= ~kIndustryElement6SectionsCompletedMask;
        _6 |= val & kIndustryElement6SectionsCompletedMask;
    }

    uint8_t IndustryElement::sectionProgress() const
    {
        return (_5 & kIndustryElement5SectionConstructionProgressMask) >> 5;
    }

    void IndustryElement::setSectionProgress(uint8_t val)
    {
        _5 &= ~kIndustryElement5SectionConstructionProgressMask;
        _5 |= (val << 5) & kIndustryElement5SectionConstructionProgressMask;
    }

    uint8_t IndustryElement::sequenceIndex() const
    {
        return _5 & kIndustryElement5TileSequenceMask;
    }

    void IndustryElement::setIsConstructed(bool val)
    {
        _0 &= ~kIndustryElement0Constructed;
        _0 |= val ? kIndustryElement0Constructed : 0;
    }

    void IndustryElement::setRandomAnimationType(uint8_t type)
    {
        _6 &= ~kIndustryElement6RandomAnimationTypeMask;
        _6 |= type;
    }

    void IndustryElement::setRandomAnimationAvailable(bool val)
    {
        _6 &= ~kIndustryElement6RandomAnimationAvailable;
        _6 |= val ? kIndustryElement6RandomAnimationAvailable : 0;
    }

    void IndustryElement::setRandomAnimationPlaying(bool val)
    {
        _6 &= ~kIndustryElement6RandomAnimationPlaying;
        _6 |= val ? kIndustryElement6RandomAnimationPlaying : 0;
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
            uint8_t newNumSections = sectionsCompleted();

            const auto progress = sectionProgress();
            if (progress == kSectionComplete)
            {
                const size_t numSections = sectionsCompleted();
                const auto parts = indObj->getBuildingParts(type);
                const auto heights = indObj->getBuildingPartHeights();
                if (parts.size() <= numSections + 1)
                {
                    ind->under_construction++;
                    if (ind->under_construction >= ind->numTiles)
                    {
                        ind->under_construction = kConstructionComplete;
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
                elIndustry.setSectionsCompleted(newNumSections);
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
            if (hasAZeroFrame && !randomAnimationPlaying())
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
                        applyToMultiTile(*this, loc, isMultiTile, [randAnim](IndustryElement& elIndustry, [[maybe_unused]] const World::Pos2& pos) {
                            elIndustry.setRandomAnimationPlaying(true);
                            elIndustry.setRandomAnimationType(randAnim);
                        });
                        AnimationManager::createAnimation(4, loc, baseZ());
                    }
                }
            }
        }

        if (ind->under_construction == kConstructionComplete)
        {
            const coord_t upperRange = isMultiTile ? 5 : 4;
            constexpr coord_t kLowerRange = 4;

            // Find all stations in range of industry building
            const auto tileStart = toTileSpace(loc);
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
    bool updateIndustryContinuousAnimation(const Animation& anim)
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
    bool updateIndustryRandomAnimation(const Animation& anim)
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
            if (!elIndustry->randomAnimationPlaying())
            {
                continue;
            }

            auto* industry = elIndustry->industry();
            const auto* indObj = industry->getObject();
            const auto type = elIndustry->buildingType();
            const auto buildingParts = indObj->getBuildingParts(type);
            const auto buildingPartAnims = indObj->getBuildingPartAnimations();
            // Guaranteed power of 2
            auto animLength = indObj->getAnimationSequence(elIndustry->randomAnimationType()).size();
            const auto isMultiTile = indObj->buildingSizeFlags & (1 << type);

            for (auto& part : buildingParts)
            {
                auto& partAnim = buildingPartAnims[part];
                if (partAnim.numFrames == 0)
                {
                    const auto animSpeed = partAnim.animationSpeed & ~(1 << 7);
                    const auto speedMask = animLength - 1;
                    if (elIndustry->randomAnimationAvailable())
                    {
                        if ((speedMask & (ScenarioManager::getScenarioTicks() >> animSpeed)) == 0)
                        {
                            applyToMultiTile(*elIndustry, anim.pos, isMultiTile, [](World::IndustryElement& elIndustry, const World::Pos2& pos) {
                                Ui::ViewportManager::invalidate(pos, elIndustry.baseHeight(), elIndustry.clearHeight(), ZoomLevel::quarter);
                                elIndustry.setRandomAnimationPlaying(false);
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
                                elIndustry.setRandomAnimationPlaying(true);
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
