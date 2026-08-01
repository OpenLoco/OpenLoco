#include "S5/S5TileElement.h"
#include "GameState.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileElementEntry.h"
#include "Map/TileManager.h"
#include "Map/TileState.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"

#include <OpenLoco/Engine/Types.hpp>

using namespace OpenLoco::World;

namespace OpenLoco::S5
{

    TileElement toSaveElement(const OpenLoco::GameState& gs, const World::TileElementEntry& entry)
    {
        const auto& ts = gs.tileState;

        TileElement out{};
        if (entry.isEmpty())
        {
            out.setBaseZ(0xFFU);
            return out;
        }

        const auto convertBase = [&](const auto& entry, const auto& elem, const auto type) {
            out.setType(type);
            out.setBaseZ(elem.baseZ());
            out.setClearZ(elem.clearZ());
            out.setFlags(elem.flags());
            out.setLast(entry.isLast());
        };

        switch (entry.type())
        {
            case World::ElementType::surface:
            {
                const auto& srcElem = ts.surface[entry.index()];
                auto& dstElem = reinterpret_cast<SurfaceElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::surface);

                dstElem.setSlope(srcElem.slope());
                dstElem.setSnowCoverage(srcElem.snowCoverage());
                dstElem.setWater(srcElem.water());
                dstElem.setUpdateTimer(srcElem.getUpdateTimer());
                dstElem.setTerrain(srcElem.terrain());
                dstElem.setGrowthStage(srcElem.getGrowthStage());
                dstElem.setVar7(srcElem.variation());
                dstElem.setIsIndustrial(srcElem.isIndustrial());
                dstElem.setType6Flag(srcElem.hasType6Flag());
                break;
            }
            case World::ElementType::track:
            {
                const auto& srcElem = ts.track[entry.index()];
                auto& dstElem = reinterpret_cast<TrackElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::track);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setHasSignal(srcElem.hasSignal());
                dstElem.setHasStationElement(srcElem.hasStationElement());
                dstElem.setTrackId(srcElem.trackId());
                dstElem.setHasGhostMods(srcElem.hasGhostMods());
                dstElem.setHasBridge(srcElem.hasBridge());
                dstElem.setSequenceIndex(srcElem.sequenceIndex());
                dstElem.setTrackObjectId(srcElem.trackObjectId());
                dstElem.setHasLevelCrossing(srcElem.hasLevelCrossing());
                dstElem.setBridge(srcElem.bridge());
                dstElem.setOwner(enumValue(srcElem.owner()));
                dstElem.setMods(srcElem.mods());
                break;
            }
            case World::ElementType::station:
            {
                const auto& srcElem = ts.station[entry.index()];
                auto& dstElem = reinterpret_cast<StationElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::station);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setSequenceIndex(srcElem.sequenceIndex());
                dstElem.setOwner(enumValue(srcElem.owner()));
                dstElem.setObjectId(srcElem.objectId());
                dstElem.setStationType(enumValue(srcElem.stationType()));
                dstElem.setStationId(enumValue(srcElem.stationId()));
                dstElem.setBuildingType(srcElem.buildingType());
                break;
            }
            case World::ElementType::signal:
            {
                const auto& srcElem = ts.signal[entry.index()];
                auto& dstElem = reinterpret_cast<SignalElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::signal);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setLeftGhost(srcElem.isLeftGhost());
                dstElem.setRightGhost(srcElem.isRightGhost());
                const auto copySide = [](SignalElement::Side& d, const World::SignalElement::Side& sr) {
                    d.setSignalObjectId(sr.signalObjectId());
                    d.setUnk4(sr.getUnk4());
                    d.setIsOccupied(sr.isOccupied());
                    d.setHasSignal(sr.hasSignal());
                    d.setFrame(sr.frame());
                    d.setAllLights(sr.allLights());
                };
                copySide(dstElem.left(), srcElem.getLeft());
                copySide(dstElem.right(), srcElem.getRight());
                break;
            }
            case World::ElementType::building:
            {
                const auto& srcElem = ts.building[entry.index()];
                auto& dstElem = reinterpret_cast<BuildingElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::building);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setIsMiscBuilding(srcElem.isMiscBuilding());
                dstElem.setConstructed(srcElem.isConstructed());
                dstElem.setObjectId(srcElem.objectId());
                dstElem.setSequenceIndex(srcElem.sequenceIndex());
                dstElem.setUnk5u(srcElem.unk5u());
                dstElem.setAge(srcElem.age());
                dstElem.setVariation(srcElem.variation());
                dstElem.setColour(enumValue(srcElem.colour()));
                break;
            }
            case World::ElementType::tree:
            {
                const auto& srcElem = ts.tree[entry.index()];
                auto& dstElem = reinterpret_cast<TreeElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::tree);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setQuadrant(srcElem.quadrant());
                dstElem.setTreeObjectId(srcElem.treeObjectId());
                dstElem.setGrowth(srcElem.growth());
                dstElem.setUnk5h(srcElem.unk5h());
                dstElem.setColour(enumValue(srcElem.colour()));
                dstElem.setSnow(srcElem.hasSnow());
                dstElem.setIsDying(srcElem.isDying());
                dstElem.setUnk7l(srcElem.unk7l());
                dstElem.setSeason(srcElem.season());
                break;
            }
            case World::ElementType::wall:
            {
                const auto& srcElem = ts.wall[entry.index()];
                auto& dstElem = reinterpret_cast<WallElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::wall);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setSlopeFlags(enumValue(srcElem.getSlopeFlags()));
                dstElem.setWallObjectId(srcElem.wallObjectId());
                dstElem.setPrimaryColour(enumValue(srcElem.getPrimaryColour()));
                dstElem.setSecondaryColour(enumValue(srcElem.getSecondaryColour()));
                dstElem.setTertiaryColour(enumValue(srcElem.getTertiaryColour()));
                break;
            }
            case World::ElementType::road:
            {
                const auto& srcElem = ts.road[entry.index()];
                auto& dstElem = reinterpret_cast<RoadElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::road);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setHasSignalElement(srcElem.hasSignalElement());
                dstElem.setHasStationElement(srcElem.hasStationElement());
                dstElem.setRoadId(srcElem.roadId());
                dstElem.setLaneOccupation(srcElem.laneOccupation());
                dstElem.setHasGhostMods(srcElem.hasGhostMods());
                dstElem.setHasBridge(srcElem.hasBridge());
                dstElem.setSequenceIndex(srcElem.sequenceIndex());
                dstElem.setLevelCrossingObjectId(srcElem.levelCrossingObjectId());
                dstElem.setRoadObjectId(srcElem.roadObjectId());
                dstElem.setUnk6l(srcElem.unk6l());
                dstElem.setBridge(srcElem.bridge());
                dstElem.setOwner(enumValue(srcElem.owner()));
                dstElem.setUnk7_10(srcElem.hasUnk7_10());
                dstElem.setHasLevelCrossing(srcElem.hasLevelCrossing());
                dstElem.setUnk7_40(srcElem.hasUnk7_40());
                dstElem.setUnk7_80(srcElem.hasUnk7_80());
                break;
            }
            case World::ElementType::industry:
            {
                const auto& srcElem = ts.industry[entry.index()];
                auto& dstElem = reinterpret_cast<IndustryElement&>(out);

                convertBase(entry, srcElem, S5::ElementType::industry);

                dstElem.setRotation(srcElem.rotation());
                dstElem.setIsConstructed(srcElem.isConstructed());
                dstElem.setIndustryId(enumValue(srcElem.industryId()));
                dstElem.setSequenceIndex(srcElem.sequenceIndex());
                dstElem.setSectionProgress(srcElem.sectionProgress());
                dstElem.setVar6_003F(srcElem.sectionsCompleted());
                dstElem.setBuildingType(srcElem.buildingType());
                dstElem.setColour(enumValue(srcElem.colour()));
                break;
            }
        }
        return out;
    }

}
