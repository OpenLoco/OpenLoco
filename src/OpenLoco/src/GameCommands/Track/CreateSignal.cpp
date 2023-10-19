#include "CreateSignal.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/SignalElement.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainSignalObject.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"

namespace OpenLoco::GameCommands
{
    static World::TrackElement* getElTrackAt(const SignalPlacementArgs& args, const World::Pos3 pos, const uint8_t index)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->baseHeight() != pos.z)
            {
                continue;
            }
            if (elTrack->unkDirection() != args.rotation)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != index)
            {
                continue;
            }
            if (elTrack->trackObjectId() != args.trackObjType)
            {
                continue;
            }
            if (elTrack->trackId() != args.trackId)
            {
                continue;
            }
            return elTrack;
        }
        return nullptr;
    };

    static bool validateTrackIsSignalCompatible(const SignalPlacementArgs& args, const std::span<const World::TrackData::PreviewTrack> trackPieces, const World::Pos3 trackStart)
    {
        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };
            auto* pieceElTrack = getElTrackAt(args, trackLoc, piece.index);
            if (pieceElTrack == nullptr)
            {
                return false;
            }
            if (pieceElTrack->hasStationElement())
            {
                setErrorText(StringIds::signals_cannot_be_built_in_stations);
                return false;
            }
            const auto connectFlags = piece.connectFlags[pieceElTrack->unkDirection()];
            auto tile = World::TileManager::get(trackLoc);
            for (auto& el : tile)
            {
                auto* otherElTrack = el.as<World::TrackElement>();
                if (otherElTrack == nullptr)
                {
                    continue;
                }
                if (otherElTrack == pieceElTrack)
                {
                    continue;
                }
                if (otherElTrack->baseZ() != pieceElTrack->baseZ())
                {
                    continue;
                }
                if (otherElTrack->isGhost())
                {
                    continue;
                }

                const auto otherConnectFlags = World::TrackData::getTrackPiece(otherElTrack->trackId())[otherElTrack->sequenceIndex()].connectFlags[otherElTrack->unkDirection()];
                if (otherConnectFlags & connectFlags)
                {
                    setErrorText(StringIds::signals_cannot_be_built_on_a_junction_2);
                    return false;
                }
            }
        }
        return true;
    }

    static currency32_t signalCost(const SignalPlacementArgs& args, const World::TrackData::PreviewTrack trackPiece0, const World::Pos3 trackStart)
    {
        const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece0.x, trackPiece0.y }, args.rotation), trackPiece0.z };
        auto* pieceElTrack = getElTrackAt(args, trackLoc, trackPiece0.index);
        if (pieceElTrack == nullptr)
        {
            return 0;
        }

        const auto* signalObj = ObjectManager::get<TrainSignalObject>(args.type);
        const auto baseCost = Economy::getInflationAdjustedCost(signalObj->costFactor, signalObj->costIndex, 10);

        currency32_t totalCost = 0;
        if (args.sides & (1U << 15))
        {
            totalCost += baseCost;
            if (pieceElTrack->hasSignal())
            {
                auto* next = pieceElTrack->next()->as<World::SignalElement>();
                if (next != nullptr && next->getLeft().hasSignal())
                {
                    if (next->getLeft().signalObjectId() == args.type)
                    {
                        totalCost -= baseCost;
                    }
                    else
                    {
                        const auto* existingSignalObj = ObjectManager::get<TrainSignalObject>(next->getLeft().signalObjectId());
                        totalCost += Economy::getInflationAdjustedCost(existingSignalObj->sellCostFactor, existingSignalObj->costIndex, 10);
                    }
                }
            }
        }
        if (args.sides & (1U << 14))
        {
            totalCost += baseCost;
            if (pieceElTrack->hasSignal())
            {
                auto* next = pieceElTrack->next()->as<World::SignalElement>();
                if (next != nullptr && next->getRight().hasSignal())
                {
                    if (next->getRight().signalObjectId() == args.type)
                    {
                        totalCost -= baseCost;
                    }
                    else
                    {
                        const auto* existingSignalObj = ObjectManager::get<TrainSignalObject>(next->getRight().signalObjectId());
                        totalCost += Economy::getInflationAdjustedCost(existingSignalObj->sellCostFactor, existingSignalObj->costIndex, 10);
                    }
                }
            }
        }
        return totalCost;
    }

    // 0x00488BDB
    static uint32_t createSignal(const SignalPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });
        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto* elTrack = getElTrackAt(args, args.pos, args.index);

        if (elTrack == nullptr)
        {
            return FAILURE;
        }

        if (elTrack->hasLevelCrossing())
        {
            setErrorText(StringIds::level_crossing_in_the_way);
            return FAILURE;
        }

        if (!sub_431E6A(elTrack->owner(), reinterpret_cast<World::TileElement*>(elTrack)))
        {
            return FAILURE;
        }

        const auto trackPieces = World::TrackData::getTrackPiece(args.trackId);
        auto& trackPiece = trackPieces[args.index];

        const auto trackStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };

        if (!validateTrackIsSignalCompatible(args, trackPieces, trackStart))
        {
            return FAILURE;
        }

        uint32_t totalCost = signalCost(args, trackPieces[0], trackStart);

        // We remove certain sides from placing when in ghost mode
        auto sides = args.sides;
        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };
            auto* pieceElTrack = getElTrackAt(args, trackLoc, piece.index);
            if (pieceElTrack == nullptr)
            {
                return FAILURE;
            }

            if (flags & Flags::ghost)
            {
                if (pieceElTrack->hasSignal())
                {
                    auto* elSignal = pieceElTrack->next()->as<World::SignalElement>();
                    if (elSignal != nullptr)
                    {
                        if (elSignal->getLeft().hasSignal())
                        {
                            sides &= ~(1U << 15);
                        }
                        if (elSignal->getRight().hasSignal())
                        {
                            sides &= ~(1U << 14);
                        }
                    }
                }
            }

            if (flags & Flags::apply)
            {
                if (!pieceElTrack->hasSignal())
                {
                    // pieceElTrack is invalid after this call!
                    auto* newSignal = World::TileManager::insertElementAfterNoReorg<World::SignalElement>(reinterpret_cast<World::TileElement*>(pieceElTrack), trackLoc, pieceElTrack->baseZ(), pieceElTrack->occupiedQuarter());
                    if (newSignal == nullptr)
                    {
                        return FAILURE;
                    }
                    pieceElTrack = newSignal->prev()->as<World::TrackElement>();
                    if (pieceElTrack == nullptr)
                    {
                        return FAILURE;
                    }
                    newSignal->setRotation(pieceElTrack->unkDirection());
                    newSignal->setGhost(flags & Flags::ghost);
                    newSignal->setFlag5(flags & Flags::flag_4);
                    newSignal->setClearZ(pieceElTrack->clearZ());
                    newSignal->getLeft() = World::SignalElement::Side{};
                    newSignal->getRight() = World::SignalElement::Side{};
                    pieceElTrack->setHasSignal(true);
                }

                auto* elSignal = pieceElTrack->next()->as<World::SignalElement>();
                if (elSignal == nullptr)
                {
                    return FAILURE;
                }
                if (sides & (1U << 15))
                {
                    if (!(flags & Flags::ghost) || !elSignal->getLeft().hasSignal())
                    {
                        auto& left = elSignal->getLeft();
                        left.setHasSignal(true);
                        elSignal->setLeftGhost(flags & Flags::ghost);
                        left.setSignalObjectId(args.type);
                        left.setFrame(0);
                        left.setAllLights(0);
                    }
                }
                if (sides & (1U << 14))
                {
                    if (!(flags & Flags::ghost) || !elSignal->getRight().hasSignal())
                    {
                        auto& right = elSignal->getRight();
                        right.setHasSignal(true);
                        elSignal->setRightGhost(flags & Flags::ghost);
                        right.setSignalObjectId(args.type);
                        right.setFrame(0);
                        right.setAllLights(0);
                    }
                }
                if (!(flags & Flags::ghost))
                {
                    World::AnimationManager::createAnimation(0, trackLoc, elSignal->baseZ());
                }
                Ui::ViewportManager::invalidate(trackLoc, elSignal->baseHeight(), elSignal->baseHeight() + 32, ZoomLevel::half);
            }
        }

        if (flags & Flags::apply)
        {
            if (!(flags & (Flags::flag_4 | Flags::ghost)))
            {
                const uint16_t tad = args.rotation | (args.trackId << 3);
                {
                    World::Track::TrackConnections connections{};

                    auto [nextLoc, nextRotation] = World::Track::getTrackConnectionEnd(args.pos, tad);
                    World::Track::getTrackConnections(nextLoc, nextRotation, connections, getUpdatingCompanyId(), args.trackObjType);
                    if (connections.size != 0)
                    {
                        Vehicles::TrackAndDirection tad2{ 0, 0 };
                        tad2._data = connections.data[0] & World::Track::AdditionalTaDFlags::basicTaDWithSignalMask;
                        Vehicles::sub_4A2AD7(nextLoc, tad2, getUpdatingCompanyId(), args.trackObjType);
                    }
                }

                auto& trackSize = World::TrackData::getUnkTrack(tad);
                auto nextTrackStart = args.pos + trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    nextTrackStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
                }

                World::Track::TrackConnections connections{};

                World::Track::getTrackConnections(nextTrackStart, World::kReverseRotation[trackSize.rotationEnd], connections, getUpdatingCompanyId(), args.trackObjType);
                if (connections.size != 0)
                {
                    Vehicles::TrackAndDirection tad2{ 0, 0 };
                    tad2._data = connections.data[0] & World::Track::AdditionalTaDFlags::basicTaDWithSignalMask;
                    Vehicles::sub_4A2AD7(nextTrackStart, tad2, getUpdatingCompanyId(), args.trackObjType);
                }
            }
        }
        return totalCost;
    }

    void createSignal(registers& regs)
    {
        regs.ebx = createSignal(SignalPlacementArgs(regs), regs.bl);
    }
}
