#include "Routing.h"
#include "Economy/Economy.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Map/AnimationManager.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/Track/TrackModSection.h"
#include "Map/TrackElement.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Vehicle.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <sfl/static_unordered_set.hpp>
#include <sfl/static_vector.hpp>

namespace OpenLoco::Vehicles
{
    using namespace OpenLoco::Interop;
    using namespace OpenLoco::World;
    using namespace OpenLoco::World::Track;

    enum class TrackNetworkSearchFlags : uint16_t
    {
        none = 0,

        excludeReverseDirection = 1U << 1,
        unk2 = 1U << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TrackNetworkSearchFlags);

    struct LocationOfInterest
    {
        World::Pos3 loc;
        uint16_t trackAndDirection; // This is a TaD with a AdditionalTaDFlags::hasSignal bit
        CompanyId company;
        uint8_t trackType;

        bool operator==(const LocationOfInterest& rhs) const
        {
            return (loc == rhs.loc) && (trackAndDirection == rhs.trackAndDirection) && (company == rhs.company) && (trackType == rhs.trackType);
        }

        TrackAndDirection::_TrackAndDirection tad() const
        {
            return TrackAndDirection::_TrackAndDirection((trackAndDirection & 0x1F8) >> 3, trackAndDirection & 0x7);
        }

        TrackAndDirection::_RoadAndDirection rad() const
        {
            return TrackAndDirection::_RoadAndDirection((trackAndDirection & 0x78) >> 3, trackAndDirection & 0x7);
        }
    };

}

template<class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<>
struct std::hash<OpenLoco::Vehicles::LocationOfInterest>
{
    std::size_t operator()(const OpenLoco::Vehicles::LocationOfInterest& interest) const noexcept
    {
        std::size_t h = 0;
        hash_combine(h, interest.loc.x);
        hash_combine(h, interest.loc.y);
        hash_combine(h, interest.loc.z);
        hash_combine(h, interest.trackAndDirection);
        return h;
    }
};

namespace OpenLoco::Vehicles
{
    constexpr size_t kTrackModHashSetSize = 0x1000;
    using LocationOfInterestHashSet = sfl::static_unordered_set<LocationOfInterest, kTrackModHashSetSize>;

    // using FilterFunction = bool (*)(const LocationOfInterest& interest);          // TODO C++20 make these concepts
    // using TransformFunction = void (*)(const LocationOfInterestHashSet& hashSet); // TODO C++20 make these concepts

    constexpr auto kNullTransformFunction = [](const LocationOfInterestHashSet&) {};

    static loco_global<uint8_t, 0x01136085> _hasDeadEnd;

    static std::optional<std::pair<World::SignalElement*, World::TrackElement*>> findSignalOnTrack(const World::Pos3& signalLoc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, const uint8_t index)
    {
        auto tile = World::TileManager::get(signalLoc);
        for (auto& el : tile)
        {
            if (el.baseZ() != signalLoc.z / 4)
            {
                continue;
            }

            auto* elTrack = el.as<TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }

            if (!elTrack->hasSignal())
            {
                continue;
            }

            if (elTrack->rotation() != trackAndDirection.cardinalDirection())
            {
                continue;
            }

            if (elTrack->sequenceIndex() != index)
            {
                continue;
            }

            if (elTrack->trackObjectId() != trackType)
            {
                continue;
            }

            if (elTrack->trackId() != trackAndDirection.id())
            {
                continue;
            }
            return std::make_pair(elTrack->next()->as<SignalElement>(), elTrack);
        }
        return std::nullopt;
    }

    // 0x0048963F but only when flags are 0xXXXX_XXXA
    SignalStateFlags getSignalState(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags)
    {
        auto trackStart = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = World::TrackData::getUnkTrack(trackAndDirection._data);
            trackStart += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            flags ^= (1ULL << 31);
        }

        auto& trackPieces = World::TrackData::getTrackPiece(trackAndDirection.id());
        auto& trackPiece = trackPieces[0];

        auto signalLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, trackAndDirection.cardinalDirection()), 0 };
        signalLoc.z += trackPiece.z;
        auto res = findSignalOnTrack(signalLoc, trackAndDirection, trackType, trackPiece.index);

        if (!res)
        {
            return SignalStateFlags::none;
        }

        auto* elSignal = res->first;

        // edx
        auto& signalSide = (flags & (1ULL << 31)) ? elSignal->getRight() : elSignal->getLeft();
        auto ret = SignalStateFlags::none;
        if (signalSide.isOccupied())
        {
            ret |= SignalStateFlags::occupied;
        }
        if (!signalSide.hasSignal())
        {
            ret |= SignalStateFlags::blockedNoRoute;
        }
        if (flags & (1ULL << 31))
        {
            if (!elSignal->getLeft().hasSignal() || elSignal->isLeftGhost())
            {
                ret |= SignalStateFlags::occupiedOneWay;
            }

            if (elSignal->isRightGhost() && elSignal->getLeft().hasSignal())
            {
                ret |= SignalStateFlags::blockedNoRoute;
            }
        }
        else
        {
            if (!elSignal->getRight().hasSignal() || elSignal->isRightGhost())
            {
                ret |= SignalStateFlags::occupiedOneWay;
            }

            if (elSignal->isLeftGhost() && elSignal->getRight().hasSignal())
            {
                ret |= SignalStateFlags::blockedNoRoute;
            }
        }
        return ret;
    }

    // 0x0048963F
    void setSignalState(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags)
    {
        const auto unk1 = flags & 0xFFFF;
        assert(unk1 != 10); // Only happens if wrong function was called call getSignalState
        auto trackStart = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = World::TrackData::getUnkTrack(trackAndDirection._data);
            trackStart += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            flags ^= (1ULL << 31);
        }

        auto& trackPieces = World::TrackData::getTrackPiece(trackAndDirection.id());
        for (auto& trackPiece : trackPieces)
        {
            auto signalLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, trackAndDirection.cardinalDirection()), 0 };
            signalLoc.z += trackPiece.z;
            auto res = findSignalOnTrack(signalLoc, trackAndDirection, trackType, trackPiece.index);

            if (!res)
            {
                // This shouldn't happen I think. Either way useful in debug to know.
                assert(false);
                return;
            }

            auto [elSignal, foundTrack] = *res;

            // edx
            auto& signalSide = (flags & (1ULL << 31)) ? elSignal->getRight() : elSignal->getLeft();
            if (unk1 == 16)
            {
                uint8_t lightStates = 0;
                if (flags & (1ULL << 30))
                {
                    lightStates |= 1 << 2;
                    if (flags & (1ULL << 28))
                    {
                        lightStates |= 1 << 0;
                    }
                }
                if (flags & (1ULL << 29))
                {
                    lightStates |= 1 << 3;
                    if (flags & (1ULL << 27))
                    {
                        lightStates |= 1 << 1;
                    }
                }

                if (signalSide.allLights() != lightStates)
                {
                    bool shouldInvalidate = false;
                    signalSide.setAllLights(lightStates);
                    if (flags & (1ULL << 31))
                    {
                        shouldInvalidate = foundTrack->sequenceIndex() == 0;
                    }
                    else
                    {
                        shouldInvalidate = foundTrack->isFlag6();
                    }

                    if (shouldInvalidate)
                    {
                        Ui::ViewportManager::invalidate(signalLoc, signalLoc.z, signalLoc.z + 24, ZoomLevel::half);
                    }
                }
            }
            else if (unk1 == 8)
            {
                signalSide.setIsOccupied(true);
            }
            else if (unk1 > 8)
            {
                signalSide.setIsOccupied(false);
            }
            else
            {
                signalSide.setUnk4(unk1 & 0x3);
                if ((unk1 & 0x3) == 0)
                {
                    // Clear lights 0b1100_0000
                    signalSide.setAllLights(signalSide.allLights() & 0xC);
                }
                bool animate = false;
                if (flags & (1ULL << 31))
                {
                    if (foundTrack->isFlag6())
                    {
                        animate = true;
                    }
                }
                else
                {
                    if (foundTrack->sequenceIndex() == 0)
                    {
                        animate = true;
                    }
                }
                if (animate)
                {
                    World::AnimationManager::createAnimation(0, signalLoc, signalLoc.z / 4);
                    Ui::ViewportManager::invalidate(signalLoc, signalLoc.z, signalLoc.z + 24, ZoomLevel::half);
                }
            }
        }
    }

    // 0x004A2AF0
    static bool isTrackOccupied(const LocationOfInterest& interest)
    {
        auto nextLoc = interest.loc;
        const auto tad = interest.tad();
        auto& trackSize = World::TrackData::getUnkTrack(tad._data);
        nextLoc += trackSize.pos;
        if (trackSize.rotationEnd < 12)
        {
            nextLoc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
        }

        auto backwardTaD = tad;
        backwardTaD.setReversed(!backwardTaD.isReversed());
        const auto startLoc = tad.isReversed() ? nextLoc : interest.loc;

        for (const auto& trackPiece : TrackData::getTrackPiece(tad.id()))
        {
            const auto rotPos = Math::Vector::rotate(Pos2{ trackPiece.x, trackPiece.y }, tad.cardinalDirection());
            const auto trackLoc = Pos2{ startLoc } + rotPos;

            for (auto* entity : EntityManager::EntityTileList(trackLoc))
            {
                auto* vehicle = entity->asBase<Vehicles::VehicleBase>();
                if (vehicle == nullptr)
                {
                    continue;
                }

                if (vehicle->has38Flags(Vehicles::Flags38::unk_0 | Vehicles::Flags38::unk_2))
                {
                    continue;
                }

                if (vehicle->getTrackLoc() == interest.loc && vehicle->getTrackAndDirection().track == tad)
                {
                    return true;
                }

                if (vehicle->getTrackLoc() == nextLoc && vehicle->getTrackAndDirection().track == backwardTaD)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // 0x004A2CE7
    static void setSignalsOccupiedState(const LocationOfInterestHashSet& hashSet)
    {
        bool isOccupied = std::ranges::any_of(hashSet, [](const auto& interest) {
            return isTrackOccupied(interest);
        });

        for (const auto& interest : hashSet)
        {
            if (!(interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal))
            {
                continue;
            }

            uint32_t flags = (1ULL << 31) | (isOccupied ? 8ULL : 9ULL);
            setSignalState(interest.loc, interest.tad(), interest.trackType, flags);
        }
    }

    static bool signalBlockFilter(const LocationOfInterest& interest)
    {
        return (interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal) != 0;
    }

    // The hash map can have a maximum of 4096 entries so the queue can't be larger than that.
    using LocationOfInterestQueue = sfl::static_vector<LocationOfInterest, 4096>;

    template<typename FilterFunction>
    static void findAllUsableTrackInNetwork(LocationOfInterestQueue& additionalTrackToCheck, const TrackNetworkSearchFlags searchFlags, const LocationOfInterest& initialInterest, FilterFunction&& filterFunction, LocationOfInterestHashSet& hashSet);

    // 0x004A313B & 0x004A35B7
    // Iterates all individual tiles of a track piece to find tracks that need inspection
    template<typename FilterFunction>
    static void findAllUsableTrackPieces(LocationOfInterestQueue& additionalTrackToCheck, const TrackNetworkSearchFlags searchFlags, const LocationOfInterest& interest, FilterFunction&& filterFunction, LocationOfInterestHashSet& hashSet)
    {
        if ((searchFlags & TrackNetworkSearchFlags::unk2) == TrackNetworkSearchFlags::none)
        {
            return;
        }

        const auto tad = interest.tad();
        auto nextLoc = interest.loc;
        if (tad.isReversed())
        {
            auto& trackSize = World::TrackData::getUnkTrack(tad._data);
            nextLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextLoc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
        }

        for (auto& piece : World::TrackData::getTrackPiece(tad.id()))
        {
            const auto connectFlags = piece.connectFlags[tad.cardinalDirection()];
            const auto pieceLoc = nextLoc + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, tad.cardinalDirection()), piece.z };
            auto tile = World::TileManager::get(pieceLoc);
            for (auto& el : tile)
            {
                if (el.baseZ() != pieceLoc.z / 4)
                {
                    continue;
                }

                auto* elTrack = el.as<TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }

                if (elTrack->isAiAllocated() || elTrack->isGhost())
                {
                    continue;
                }

                const auto& targetPiece = World::TrackData::getTrackPiece(elTrack->trackId())[elTrack->sequenceIndex()];
                const auto targetConnectFlags = targetPiece.connectFlags[elTrack->rotation()];
                if ((targetConnectFlags & connectFlags) == 0)
                {
                    continue;
                }

                // If identical then no need to keep checking
                if (elTrack->rotation() == tad.cardinalDirection()
                    && elTrack->sequenceIndex() == piece.index
                    && elTrack->trackObjectId() == interest.trackType
                    && elTrack->trackId() == tad.id())
                {
                    continue;
                }

                const auto startTargetPos2 = World::Pos2{ pieceLoc } - Math::Vector::rotate(World::Pos2{ targetPiece.x, targetPiece.y }, elTrack->rotation());
                const auto startTargetPos = World::Pos3{ startTargetPos2, static_cast<int16_t>(elTrack->baseHeight() - targetPiece.z) };
                TrackAndDirection::_TrackAndDirection tad2(elTrack->trackId(), elTrack->rotation());
                LocationOfInterest newInterest{ startTargetPos, tad2._data, elTrack->owner(), elTrack->trackObjectId() };

                if (!hashSet.full() && hashSet.insert(newInterest).second)
                {
                    if (!filterFunction(newInterest))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, searchFlags, newInterest, filterFunction, hashSet);
                        additionalTrackToCheck.push_back(newInterest);
                    }
                }

                auto& trackSize = World::TrackData::getUnkTrack(tad2._data);
                auto endTargetPos = startTargetPos + trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    endTargetPos -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
                }

                tad2.setReversed(!tad2.isReversed());
                LocationOfInterest newInterestR{ endTargetPos, tad2._data, elTrack->owner(), elTrack->trackObjectId() };

                if (!hashSet.full() && hashSet.insert(newInterestR).second)
                {
                    if (!filterFunction(newInterestR))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, searchFlags, newInterestR, filterFunction, hashSet);
                        additionalTrackToCheck.push_back(newInterestR);
                    }
                }
            }
        }
    }

    // 0x004A2FE6 & 0x004A3462
    template<typename FilterFunction>
    static void findAllUsableTrackInNetwork(LocationOfInterestQueue& additionalTrackToCheck, const TrackNetworkSearchFlags searchFlags, const LocationOfInterest& initialInterest, FilterFunction&& filterFunction, LocationOfInterestHashSet& hashSet)
    {
        const auto [trackEndLoc, trackEndRotation] = World::Track::getTrackConnectionEnd(initialInterest.loc, initialInterest.tad()._data);
        auto tc = World::Track::getTrackConnections(trackEndLoc, trackEndRotation, initialInterest.company, initialInterest.trackType, 0, 0);

        if (!tc.connections.empty())
        {
            for (auto c : tc.connections)
            {
                uint16_t trackAndDirection2 = c & World::Track::AdditionalTaDFlags::basicTaDWithSignalMask;
                LocationOfInterest interest{ trackEndLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (!hashSet.full() && hashSet.insert(interest).second)
                {
                    if (!filterFunction(interest))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, searchFlags, interest, filterFunction, hashSet);
                        additionalTrackToCheck.push_back(interest);
                    }
                }
            }
        }
        else
        {
            _hasDeadEnd = *_hasDeadEnd | (1 << 0);
        }

        if ((searchFlags & TrackNetworkSearchFlags::excludeReverseDirection) == TrackNetworkSearchFlags::none)
        {
            // odd logic here clearing a flag in a branch that can never hit
            auto nextLoc = initialInterest.loc;
            auto& trackSize = World::TrackData::getUnkTrack(initialInterest.tad()._data);
            nextLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextLoc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }

            const auto rotation = World::kReverseRotation[trackSize.rotationEnd];
            auto tc2 = World::Track::getTrackConnections(nextLoc, rotation, initialInterest.company, initialInterest.trackType, 0, 0);
            for (auto c : tc2.connections)
            {
                uint16_t trackAndDirection2 = c & World::Track::AdditionalTaDFlags::basicTaDWithSignalMask;
                LocationOfInterest interest{ nextLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (!hashSet.full() && hashSet.insert(interest).second)
                {
                    if (!filterFunction(interest))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, searchFlags, interest, filterFunction, hashSet);
                        additionalTrackToCheck.push_back(interest);
                    }
                }
            }
        }
    }

    // 0x004A2E46 & 0x004A2DE4
    template<typename FilterFunction, typename TransformFunction>
    static void findAllTracksFilterTransform(LocationOfInterestHashSet& interestMap, TrackNetworkSearchFlags searchFlags, const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType, FilterFunction&& filterFunction, TransformFunction&& transformFunction)
    {
        // _1135F06 = &interestMap;
        // _filterFunction = filterFunction;
        // _transformFunction = transformFunction;
        // _1135F0A = 0;

        // Note: This function and its call chain findAllUsableTrackInNetwork and findAllUsableTrackPieces have been modified
        // to not be recursive anymore.
        LocationOfInterestQueue trackToCheck{ LocationOfInterest{ loc, trackAndDirection._data, company, trackType } };
        while (!trackToCheck.empty())
        {
            const auto interest = trackToCheck.back();
            trackToCheck.pop_back();
            findAllUsableTrackInNetwork(trackToCheck, searchFlags, interest, filterFunction, interestMap);
        }
        transformFunction(interestMap);
    }

    template<typename FilterFunction>
    static void findAllUsableRoadInNetwork(LocationOfInterestQueue& additionalRoadToCheck, const LocationOfInterest& initialInterest, FilterFunction&& filterFunction, LocationOfInterestHashSet& hashSet);

    // 0x00479EFA
    // Iterates all individual tiles of a road piece to find roads that need inspection
    template<typename FilterFunction>
    static void findAllUsableRoadPieces(LocationOfInterestQueue& additionalRoadToCheck, const TrackNetworkSearchFlags searchFlags, const LocationOfInterest& interest, FilterFunction&& filterFunction, LocationOfInterestHashSet& hashSet)
    {
        if ((searchFlags & TrackNetworkSearchFlags::unk2) == TrackNetworkSearchFlags::none)
        {
            return;
        }

        const auto rad = interest.rad();
        auto nextLoc = interest.loc;
        if (rad.isReversed())
        {
            auto& roadSize = World::TrackData::getUnkRoad(rad._data);
            nextLoc += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                nextLoc -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
        }

        for (auto& piece : World::TrackData::getRoadPiece(rad.id()))
        {
            const auto connectFlags = piece.connectFlags[rad.cardinalDirection()];
            const auto pieceLoc = nextLoc + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rad.cardinalDirection()), piece.z };
            auto tile = World::TileManager::get(pieceLoc);
            for (auto& el : tile)
            {
                if (el.baseZ() != pieceLoc.z / 4)
                {
                    continue;
                }

                auto* elRoad = el.as<RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }

                if (elRoad->isAiAllocated() || elRoad->isGhost())
                {
                    continue;
                }

                const auto& targetPiece = World::TrackData::getRoadPiece(elRoad->roadId())[elRoad->sequenceIndex()];
                const auto targetConnectFlags = targetPiece.connectFlags[elRoad->rotation()];
                if ((targetConnectFlags & connectFlags) == 0)
                {
                    continue;
                }

                // If identical then no need to keep checking
                if (elRoad->rotation() == rad.cardinalDirection()
                    && elRoad->sequenceIndex() == piece.index
                    && elRoad->roadObjectId() == interest.trackType
                    && elRoad->roadId() == rad.id())
                {
                    continue;
                }

                const auto startTargetPos2 = World::Pos2{ pieceLoc } - Math::Vector::rotate(World::Pos2{ targetPiece.x, targetPiece.y }, elRoad->rotation());
                const auto startTargetPos = World::Pos3{ startTargetPos2, static_cast<int16_t>(elRoad->baseHeight() - targetPiece.z) };
                TrackAndDirection::_RoadAndDirection tad2(elRoad->roadId(), elRoad->rotation());
                LocationOfInterest newInterest{ startTargetPos, tad2._data, elRoad->owner(), elRoad->roadObjectId() };

                if (!hashSet.full() && hashSet.insert(newInterest).second)
                {
                    if (!filterFunction(newInterest))
                    {
                        findAllUsableRoadPieces(additionalRoadToCheck, searchFlags, newInterest, filterFunction, hashSet);
                        additionalRoadToCheck.push_back(newInterest);
                    }
                }

                auto& roadSize = World::TrackData::getUnkRoad(tad2._data);
                auto endTargetPos = startTargetPos + roadSize.pos;
                if (roadSize.rotationEnd < 12)
                {
                    endTargetPos -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
                }

                tad2.setReversed(!tad2.isReversed());
                LocationOfInterest newInterestR{ endTargetPos, tad2._data, elRoad->owner(), elRoad->roadObjectId() };

                if (!hashSet.full() && hashSet.insert(newInterestR).second)
                {
                    if (!filterFunction(newInterestR))
                    {
                        findAllUsableRoadPieces(additionalRoadToCheck, searchFlags, newInterestR, filterFunction, hashSet);
                        additionalRoadToCheck.push_back(newInterestR);
                    }
                }
            }
        }
    }

    // 0x00479DB1
    template<typename FilterFunction>
    static void findAllUsableRoadInNetwork(LocationOfInterestQueue& additionalRoadToCheck, const TrackNetworkSearchFlags searchFlags, const LocationOfInterest& initialInterest, FilterFunction&& filterFunction, LocationOfInterestHashSet& hashSet)
    {
        const auto [roadEndLoc, roadEndRotation] = World::Track::getRoadConnectionEnd(initialInterest.loc, initialInterest.rad()._data);
        auto tc = World::Track::getRoadConnections(roadEndLoc, roadEndRotation, initialInterest.company, initialInterest.trackType, 0, 0);

        if (!tc.connections.empty())
        {
            for (auto c : tc.connections)
            {
                uint16_t trackAndDirection2 = c & World::Track::AdditionalTaDFlags::basicRaDWithSignalMask;
                LocationOfInterest interest{ roadEndLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (!hashSet.full() && hashSet.insert(interest).second)
                {
                    if (!filterFunction(interest))
                    {
                        findAllUsableRoadPieces(additionalRoadToCheck, searchFlags, interest, filterFunction, hashSet);
                        additionalRoadToCheck.push_back(interest);
                    }
                }
            }
        }
        else
        {
            _hasDeadEnd = *_hasDeadEnd | (1 << 0);
        }

        if ((searchFlags & TrackNetworkSearchFlags::excludeReverseDirection) == TrackNetworkSearchFlags::none)
        {
            // odd logic here clearing a flag in a branch that can never hit
            auto nextLoc = initialInterest.loc;
            auto& roadSize = World::TrackData::getUnkRoad(initialInterest.rad()._data);
            nextLoc += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                nextLoc -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }

            const auto rotation = World::kReverseRotation[roadSize.rotationEnd];
            auto tc2 = World::Track::getRoadConnections(nextLoc, rotation, initialInterest.company, initialInterest.trackType, 0, 0);
            for (auto c : tc2.connections)
            {
                uint16_t trackAndDirection2 = c & World::Track::AdditionalTaDFlags::basicRaDWithSignalMask;
                LocationOfInterest interest{ nextLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (!hashSet.full() && hashSet.insert(interest).second)
                {
                    if (!filterFunction(interest))
                    {
                        findAllUsableRoadPieces(additionalRoadToCheck, searchFlags, interest, filterFunction, hashSet);
                        additionalRoadToCheck.push_back(interest);
                    }
                }
            }
        }
    }

    template<typename FilterFunction, typename TransformFunction>
    static void findAllRoadsFilterTransform(LocationOfInterestHashSet& interestMap, TrackNetworkSearchFlags searchFlags, const World::Pos3& loc, const TrackAndDirection::_RoadAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType, FilterFunction&& filterFunction, TransformFunction&& transformFunction)
    {
        // _1135F06 = &interestMap;
        // _filterFunction = filterFunction;
        // _transformFunction = transformFunction;
        // _1135F0A = 0;

        // Note: This function and its call chain findAllUsableTrackInNetwork and findAllUsableTrackPieces have been modified
        // to not be recursive anymore.
        LocationOfInterestQueue roadToCheck{ LocationOfInterest{ loc, trackAndDirection._data, company, trackType } };
        while (!roadToCheck.empty())
        {
            const auto interest = roadToCheck.back();
            roadToCheck.pop_back();
            findAllUsableRoadInNetwork(roadToCheck, searchFlags, interest, filterFunction, interestMap);
        }
        transformFunction(interestMap);
    }

    // 0x004A2AD7
    void updateSignalOccupiedInBlock(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        LocationOfInterestHashSet interestMap{};

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::unk2,
            loc,
            trackAndDirection,
            company,
            trackType,
            signalBlockFilter,
            setSignalsOccupiedState);
    }

    // 0x004A2A39
    bool isBlockOccupied(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        // 0x001135F88
        bool isOccupied = false;

        auto transformFunction = [&isOccupied](const LocationOfInterestHashSet& hashSet) {
            isOccupied = std::ranges::any_of(hashSet, [](const auto& interest) {
                return isTrackOccupied(interest);
            });
        };

        LocationOfInterestHashSet interestMap{};

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::unk2,
            loc,
            trackAndDirection,
            company,
            trackType,
            signalBlockFilter,
            transformFunction);
        return isOccupied;
    }

    void setReverseSignalOccupiedInBlock(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        LocationOfInterestHashSet interestMap{};

        auto transformFunction = [](const LocationOfInterestHashSet& set) {
            for (auto& interest : set)
            {
                if (!(interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal))
                {
                    continue;
                }

                setSignalState(interest.loc, interest.tad(), interest.trackType, (1ULL << 31) | (8));
            }
        };

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::unk2,
            loc,
            trackAndDirection,
            company,
            trackType,
            signalBlockFilter,
            transformFunction);
    }

    // 0x004A2A58
    uint8_t sub_4A2A58(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        // 0x001135F88
        uint16_t unk = 0;
        LocationOfInterestHashSet interestMap{};

        // 0x004A2D4C
        auto transformFunction = [&unk](const LocationOfInterestHashSet& set) {
            for (auto& interest : set)
            {
                if (!(interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal))
                {
                    continue;
                }

                if ((getSignalState(interest.loc, interest.tad(), interest.trackType, (1ULL << 31)) & SignalStateFlags::occupied) != SignalStateFlags::none)
                {
                    unk |= (1 << 0);
                }
                else
                {
                    unk |= (1 << 1);
                }
            }
        };

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::unk2,
            loc,
            trackAndDirection,
            company,
            trackType,
            signalBlockFilter,
            transformFunction);

        return unk;
    }

    uint8_t sub_4A2A77(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        bool isUnk = false;
        _hasDeadEnd = 0;

        // 0x004A2AA1
        auto transformFunction = [&isUnk](const LocationOfInterestHashSet& hashSet) {
            isUnk = std::ranges::any_of(hashSet, [](const auto& interest) {
                if (!(interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal))
                {
                    return false;
                }
                const auto signalState = getSignalState(interest.loc, interest.tad(), interest.trackType, 0);

                const bool occupiedOneWay = (signalState & SignalStateFlags::occupiedOneWay) != SignalStateFlags::none;
                // ??? Not sure why we are doing this
                const bool clearRoute = ((signalState & SignalStateFlags::blockedNoRoute) == SignalStateFlags::none)
                    && ((signalState & SignalStateFlags::occupied) == SignalStateFlags::none);
                return occupiedOneWay || clearRoute;
            });
        };
        LocationOfInterestHashSet interestMap{};

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::excludeReverseDirection,
            loc,
            trackAndDirection,
            company,
            trackType,
            signalBlockFilter,
            transformFunction);

        uint16_t routingTransformData = isUnk ? (1U << 0) : 0;
        if (_hasDeadEnd & (1U << 0))
        {
            routingTransformData |= (1U << 1);
        }
        return routingTransformData;
    }

    // 0x004A5D94
    static bool applyTrackModToTrack(const LocationOfInterest& interest, const uint8_t flags, LocationOfInterestHashSet* hashSet, ModSection modSelection, uint8_t trackObjectId, uint8_t trackModObjectIds, currency32_t& totalCost, CompanyId companyId, bool& hasFailedAllPlacement)
    {
        // If not in single segment mode then we should add the reverse
        // direction of track to the hashset to prevent it being visited.
        // This is because track mods do not have directions so applying
        // to the reverse would do nothing (or worse double spend)
        if (hashSet != nullptr)
        {
            LocationOfInterest reverseInterest = interest;
            reverseInterest.loc = interest.loc;
            const auto tad = interest.tad();
            auto& trackSize = World::TrackData::getUnkTrack(tad._data);
            reverseInterest.loc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                reverseInterest.loc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            reverseInterest.trackAndDirection ^= (1 << 2); // Reverse flag

            if (!hashSet->full())
            {
                hashSet->insert(reverseInterest);
            }
        }

        auto* trackObj = ObjectManager::get<TrackObject>(trackObjectId);
        bool placementFailure = false;

        for (auto i = 0; i < 4; ++i)
        {
            if (!(trackModObjectIds & (1U << i)))
            {
                continue;
            }

            auto* trackModObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);

            const auto pieceFlags = TrackData::getTrackMiscData(interest.tad().id()).compatibleFlags;
            if ((trackModObj->trackPieces & pieceFlags) != pieceFlags)
            {
                //_1135F64 |= (1 << 0); placement failed at least once
                placementFailure = true;
                break;
            }
        }

        if (!placementFailure)
        {
            auto trackStart = interest.loc;
            const auto tad = interest.tad();
            if (tad.isReversed())
            {
                auto& trackSize = World::TrackData::getUnkTrack(tad._data);
                trackStart += trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    trackStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
                }
            }

            for (auto& trackPiece : TrackData::getTrackPiece(tad.id()))
            {
                auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, tad.cardinalDirection()), 0 };
                trackLoc.z += trackPiece.z;

                auto tile = TileManager::get(trackLoc);
                World::TrackElement* elTrack = nullptr;
                for (auto& el : tile)
                {
                    elTrack = el.as<World::TrackElement>();
                    if (elTrack == nullptr)
                    {
                        continue;
                    }
                    if (elTrack->baseHeight() != trackLoc.z)
                    {
                        continue;
                    }
                    if (elTrack->rotation() != tad.cardinalDirection())
                    {
                        continue;
                    }
                    if (elTrack->sequenceIndex() != trackPiece.index)
                    {
                        continue;
                    }
                    if (elTrack->trackObjectId() != trackObjectId)
                    {
                        continue;
                    }
                    if (elTrack->trackId() != tad.id())
                    {
                        continue;
                    }

                    break;
                }
                if (elTrack == nullptr)
                {
                    placementFailure = true;
                    break;
                }

                if (trackPiece.index == 0)
                {
                    // increment successful placement count
                    hasFailedAllPlacement = false;
                    // For each track mod
                    //   Get mod cost (changes depending on track id)
                    for (auto i = 0; i < 4; ++i)
                    {
                        if (!(trackModObjectIds & (1U << i)))
                        {
                            continue;
                        }
                        if (elTrack->mods() & (1U << i))
                        {
                            continue;
                        }

                        auto* trackModObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                        const auto baseCost = Economy::getInflationAdjustedCost(trackModObj->buildCostFactor, trackModObj->costIndex, 10);
                        const auto cost = (baseCost * TrackData::getTrackMiscData(tad.id()).costFactor) / 256;
                        totalCost += cost;
                    }
                }

                if (flags & GameCommands::Flags::apply)
                {
                    bool invalidate = false;
                    if (flags & GameCommands::Flags::ghost)
                    {
                        if (CompanyManager::getControllingId() == companyId)
                        {
                            elTrack->setHasGhostMods(true);
                            invalidate = true;
                        }
                    }
                    else
                    {
                        for (auto i = 0; i < 4; ++i)
                        {
                            if (trackModObjectIds & (1U << i))
                            {
                                elTrack->setMod(i, true);
                            }
                        }
                        invalidate = true;
                    }
                    if (invalidate)
                    {
                        Ui::ViewportManager::invalidate(trackLoc, elTrack->baseHeight(), elTrack->clearHeight(), ZoomLevel::half);
                    }
                }
            }
        }

        if (modSelection == ModSection::block)
        {
            return interest.trackAndDirection & Track::AdditionalTaDFlags::hasSignal;
        }
        return false;
    }

    // 0x004A6136
    static bool removeTrackModToTrack(const LocationOfInterest& interest, const uint8_t flags, LocationOfInterestHashSet* hashSet, ModSection modSelection, uint8_t trackObjectId, uint8_t trackModObjectIds, currency32_t& totalCost, CompanyId companyId)
    {
        // If not in single segment mode then we should add the reverse
        // direction of track to the hashset to prevent it being visited.
        // This is because track mods do not have directions so applying
        // to the reverse would do nothing (or worse double spend)
        if (hashSet != nullptr)
        {
            LocationOfInterest reverseInterest = interest;
            reverseInterest.loc = interest.loc;
            const auto tad = interest.tad();
            auto& trackSize = World::TrackData::getUnkTrack(tad._data);
            reverseInterest.loc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                reverseInterest.loc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            reverseInterest.trackAndDirection ^= (1 << 2); // Reverse flag

            if (!hashSet->full())
            {
                hashSet->insert(reverseInterest);
            }
        }

        auto* trackObj = ObjectManager::get<TrackObject>(trackObjectId);

        auto trackStart = interest.loc;
        const auto tad = interest.tad();
        if (tad.isReversed())
        {
            auto& trackSize = World::TrackData::getUnkTrack(tad._data);
            trackStart += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
        }

        for (auto& trackPiece : TrackData::getTrackPiece(tad.id()))
        {
            auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, tad.cardinalDirection()), 0 };
            trackLoc.z += trackPiece.z;

            auto tile = TileManager::get(trackLoc);
            World::TrackElement* elTrack = nullptr;
            for (auto& el : tile)
            {
                elTrack = el.as<World::TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }
                if (elTrack->baseHeight() != trackLoc.z)
                {
                    continue;
                }
                if (elTrack->rotation() != tad.cardinalDirection())
                {
                    continue;
                }
                if (elTrack->sequenceIndex() != trackPiece.index)
                {
                    continue;
                }
                if (elTrack->trackObjectId() != trackObjectId)
                {
                    continue;
                }
                if (elTrack->trackId() != tad.id())
                {
                    continue;
                }

                break;
            }
            if (elTrack == nullptr)
            {
                break;
            }

            if (trackPiece.index == 0)
            {
                // For each track mod
                //   Get mod cost (changes depending on track id)
                for (auto i = 0; i < 4; ++i)
                {
                    if (!(trackModObjectIds & (1U << i)))
                    {
                        continue;
                    }
                    if (!(elTrack->mods() & (1U << i)))
                    {
                        continue;
                    }

                    auto* trackModObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                    const auto baseCost = Economy::getInflationAdjustedCost(trackModObj->sellCostFactor, trackModObj->costIndex, 10);
                    const auto cost = (baseCost * TrackData::getTrackMiscData(tad.id()).costFactor) / 256;
                    totalCost += cost;
                }
            }

            if (flags & GameCommands::Flags::apply)
            {
                bool invalidate = false;
                if (flags & GameCommands::Flags::ghost)
                {
                    if (CompanyManager::getControllingId() == companyId)
                    {
                        elTrack->setHasGhostMods(false);
                        invalidate = true;
                    }
                }
                else
                {
                    for (auto i = 0; i < 4; ++i)
                    {
                        if (trackModObjectIds & (1U << i))
                        {
                            elTrack->setMod(i, false);
                        }
                    }
                    invalidate = true;
                }
                if (invalidate)
                {
                    Ui::ViewportManager::invalidate(trackLoc, elTrack->baseHeight(), elTrack->clearHeight(), ZoomLevel::half);
                }
            }
        }

        if (modSelection == ModSection::block)
        {
            return interest.trackAndDirection & Track::AdditionalTaDFlags::hasSignal;
        }
        return false;
    }

    ApplyTrackModsResult applyTrackModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_TrackAndDirection trackAndDirection, CompanyId company, uint8_t trackType, uint8_t flags, ModSection modSelection, uint8_t trackModObjIds)
    {
        ApplyTrackModsResult result{};
        result.cost = 0;
        result.allPlacementsFailed = true;
        result.networkTooComplex = false;

        if (modSelection == Track::ModSection::single)
        {
            LocationOfInterest interest{ pos, trackAndDirection._data, company, trackType };
            applyTrackModToTrack(interest, flags, nullptr, modSelection, trackType, trackModObjIds, result.cost, company, result.allPlacementsFailed);
            return result;
        }

        LocationOfInterestHashSet interestHashSet{};

        auto filterFunction = [flags, modSelection, trackType, trackModObjIds, &result, company, &interestHashSet](const LocationOfInterest& interest) {
            return applyTrackModToTrack(interest, flags, &interestHashSet, modSelection, trackType, trackModObjIds, result.cost, company, result.allPlacementsFailed);
        };
        findAllTracksFilterTransform(interestHashSet, TrackNetworkSearchFlags::none, pos, trackAndDirection, company, trackType, filterFunction, kNullTransformFunction);
        result.networkTooComplex = interestHashSet.available() < 100;
        return result;
    }

    currency32_t removeTrackModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_TrackAndDirection trackAndDirection, CompanyId company, uint8_t trackType, uint8_t flags, ModSection modSelection, uint8_t trackModObjIds)
    {
        currency32_t cost = 0;
        if (modSelection == Track::ModSection::single)
        {
            LocationOfInterest interest{ pos, trackAndDirection._data, company, trackType };
            removeTrackModToTrack(interest, flags, nullptr, modSelection, trackType, trackModObjIds, cost, company);
            return cost;
        }

        LocationOfInterestHashSet interestHashSet{};

        auto filterFunction = [flags, modSelection, trackType, trackModObjIds, &cost, company, &interestHashSet](const LocationOfInterest& interest) {
            return removeTrackModToTrack(interest, flags, &interestHashSet, modSelection, trackType, trackModObjIds, cost, company);
        };
        findAllTracksFilterTransform(interestHashSet, TrackNetworkSearchFlags::none, pos, trackAndDirection, company, trackType, filterFunction, kNullTransformFunction);
        return cost;
    }

    // 0x0047A5E6
    static bool applyRoadModToRoad(const LocationOfInterest& interest, const uint8_t flags, LocationOfInterestHashSet* hashSet, ModSection modSelection, uint8_t roadObjectId, uint8_t roadModObjectIds, currency32_t& totalCost, CompanyId companyId, bool& hasFailedAllPlacement)
    {
        // If not in single segment mode then we should add the reverse
        // direction of track to the hashset to prevent it being visited.
        // This is because track mods do not have directions so applying
        // to the reverse would do nothing (or worse double spend)
        if (hashSet != nullptr)
        {
            LocationOfInterest reverseInterest = interest;
            reverseInterest.loc = interest.loc;
            const auto rad = interest.rad();
            auto& roadSize = World::TrackData::getUnkRoad(rad._data);
            reverseInterest.loc += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                reverseInterest.loc -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
            reverseInterest.trackAndDirection ^= (1 << 2); // Reverse flag

            if (!hashSet->full())
            {
                hashSet->insert(reverseInterest);
            }
        }

        auto* roadObj = ObjectManager::get<RoadObject>(roadObjectId);
        bool placementFailure = false;

        for (auto i = 0; i < 2; ++i)
        {
            if (!(roadModObjectIds & (1U << i)))
            {
                continue;
            }

            auto* roadModObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);

            const auto pieceFlags = TrackData::getRoadMiscData(interest.rad().id()).compatibleFlags;
            if ((roadModObj->roadPieces & pieceFlags) != pieceFlags)
            {
                //_1135F64 |= (1 << 0); placement failed at least once
                placementFailure = true;
                break;
            }
        }

        if (!placementFailure)
        {
            auto roadStart = interest.loc;
            const auto rad = interest.rad();
            if (rad.isReversed())
            {
                auto& roadSize = World::TrackData::getUnkRoad(rad._data);
                roadStart += roadSize.pos;
                if (roadSize.rotationEnd < 12)
                {
                    roadStart -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
                }
            }

            for (auto& roadPiece : TrackData::getRoadPiece(rad.id()))
            {
                auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece.x, roadPiece.y }, rad.cardinalDirection()), 0 };
                roadLoc.z += roadPiece.z;

                auto tile = TileManager::get(roadLoc);
                World::RoadElement* elRoad = nullptr;
                for (auto& el : tile)
                {
                    elRoad = el.as<World::RoadElement>();
                    if (elRoad == nullptr)
                    {
                        continue;
                    }
                    if (elRoad->baseHeight() != roadLoc.z)
                    {
                        continue;
                    }
                    if (elRoad->rotation() != rad.cardinalDirection())
                    {
                        continue;
                    }
                    if (elRoad->sequenceIndex() != roadPiece.index)
                    {
                        continue;
                    }
                    if (elRoad->roadObjectId() != roadObjectId)
                    {
                        continue;
                    }
                    if (elRoad->roadId() != rad.id())
                    {
                        continue;
                    }

                    break;
                }
                if (elRoad == nullptr)
                {
                    placementFailure = true;
                    break;
                }

                if (roadPiece.index == 0)
                {
                    // increment successful placement count
                    hasFailedAllPlacement = false;
                    // For each road mod
                    //   Get mod cost (changes depending on road id)
                    for (auto i = 0; i < 2; ++i)
                    {
                        if (!(roadModObjectIds & (1U << i)))
                        {
                            continue;
                        }
                        if (elRoad->mods() & (1U << i))
                        {
                            continue;
                        }

                        auto* roadModObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);
                        const auto baseCost = Economy::getInflationAdjustedCost(roadModObj->buildCostFactor, roadModObj->costIndex, 10);
                        const auto cost = (baseCost * TrackData::getRoadMiscData(rad.id()).costFactor) / 256;
                        totalCost += cost;
                    }
                }

                if (flags & GameCommands::Flags::apply)
                {
                    bool invalidate = false;
                    if (flags & GameCommands::Flags::ghost)
                    {
                        if (CompanyManager::getControllingId() == companyId)
                        {
                            elRoad->setHasGhostMods(true);
                            invalidate = true;
                        }
                    }
                    else
                    {
                        for (auto i = 0; i < 2; ++i)
                        {
                            if (roadModObjectIds & (1U << i))
                            {
                                elRoad->setMod(i, true);
                            }
                        }
                        invalidate = true;
                    }
                    if (invalidate)
                    {
                        Ui::ViewportManager::invalidate(roadLoc, elRoad->baseHeight(), elRoad->clearHeight(), ZoomLevel::half);
                    }
                }
            }
        }

        if (modSelection == ModSection::block)
        {
            return interest.trackAndDirection & Track::AdditionalTaDFlags::hasSignal;
        }
        return false;
    }

    ApplyTrackModsResult applyRoadModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_RoadAndDirection roadAndDirection, CompanyId company, uint8_t roadType, uint8_t flags, ModSection modSelection, uint8_t roadModObjIds)
    {
        ApplyTrackModsResult result{};
        result.cost = 0;
        result.allPlacementsFailed = true;
        result.networkTooComplex = false;

        if (modSelection == Track::ModSection::single)
        {
            LocationOfInterest interest{ pos, roadAndDirection._data, company, roadType };
            applyRoadModToRoad(interest, flags, nullptr, modSelection, roadType, roadModObjIds, result.cost, company, result.allPlacementsFailed);
            return result;
        }

        LocationOfInterestHashSet interestHashSet{};

        auto filterFunction = [flags, modSelection, roadType, roadModObjIds, &result, company, &interestHashSet](const LocationOfInterest& interest) {
            return applyRoadModToRoad(interest, flags, &interestHashSet, modSelection, roadType, roadModObjIds, result.cost, company, result.allPlacementsFailed);
        };
        findAllRoadsFilterTransform(interestHashSet, TrackNetworkSearchFlags::none, pos, roadAndDirection, company, roadType, filterFunction, kNullTransformFunction);
        result.networkTooComplex = interestHashSet.available() < 100;
        return result;
    }

    // 0x0047A8F0
    static bool removeRoadModToTrack(const LocationOfInterest& interest, const uint8_t flags, LocationOfInterestHashSet* hashSet, ModSection modSelection, uint8_t roadObjectId, uint8_t roadModObjectIds, currency32_t& totalCost, CompanyId companyId)
    {
        // If not in single segment mode then we should add the reverse
        // direction of track to the hashset to prevent it being visited.
        // This is because track mods do not have directions so applying
        // to the reverse would do nothing (or worse double spend)
        if (hashSet != nullptr)
        {
            LocationOfInterest reverseInterest = interest;
            reverseInterest.loc = interest.loc;
            const auto rad = interest.rad();
            auto& roadSize = World::TrackData::getUnkRoad(rad._data);
            reverseInterest.loc += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                reverseInterest.loc -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
            reverseInterest.trackAndDirection ^= (1 << 2); // Reverse flag

            if (!hashSet->full())
            {
                hashSet->insert(reverseInterest);
            }
        }

        auto* roadObj = ObjectManager::get<RoadObject>(roadObjectId);

        auto roadStart = interest.loc;
        const auto rad = interest.rad();
        if (rad.isReversed())
        {
            auto& roadSize = World::TrackData::getUnkRoad(rad._data);
            roadStart += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                roadStart -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
        }

        for (auto& roadPiece : TrackData::getRoadPiece(rad.id()))
        {
            auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece.x, roadPiece.y }, rad.cardinalDirection()), 0 };
            roadLoc.z += roadPiece.z;

            auto tile = TileManager::get(roadLoc);
            World::RoadElement* elRoad = nullptr;
            for (auto& el : tile)
            {
                elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->baseHeight() != roadLoc.z)
                {
                    continue;
                }
                if (elRoad->rotation() != rad.cardinalDirection())
                {
                    continue;
                }
                if (elRoad->sequenceIndex() != roadPiece.index)
                {
                    continue;
                }
                if (elRoad->roadObjectId() != roadObjectId)
                {
                    continue;
                }
                if (elRoad->roadId() != rad.id())
                {
                    continue;
                }

                break;
            }
            if (elRoad == nullptr)
            {
                break;
            }

            if (roadPiece.index == 0)
            {
                // For each track mod
                //   Get mod cost (changes depending on track id)
                for (auto i = 0; i < 2; ++i)
                {
                    if (!(roadModObjectIds & (1U << i)))
                    {
                        continue;
                    }
                    if (!(elRoad->mods() & (1U << i)))
                    {
                        continue;
                    }

                    auto* roadModObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);
                    const auto baseCost = Economy::getInflationAdjustedCost(roadModObj->sellCostFactor, roadModObj->costIndex, 10);
                    const auto cost = (baseCost * TrackData::getRoadMiscData(rad.id()).costFactor) / 256;
                    totalCost += cost;
                }
            }

            if (flags & GameCommands::Flags::apply)
            {
                bool invalidate = false;
                if (flags & GameCommands::Flags::ghost)
                {
                    if (CompanyManager::getControllingId() == companyId)
                    {
                        elRoad->setHasGhostMods(false);
                        invalidate = true;
                    }
                }
                else
                {
                    for (auto i = 0; i < 4; ++i)
                    {
                        if (roadModObjectIds & (1U << i))
                        {
                            elRoad->setMod(i, false);
                        }
                    }
                    invalidate = true;
                }
                if (invalidate)
                {
                    Ui::ViewportManager::invalidate(roadLoc, elRoad->baseHeight(), elRoad->clearHeight(), ZoomLevel::half);
                }
            }
        }

        if (modSelection == ModSection::block)
        {
            return interest.trackAndDirection & Track::AdditionalTaDFlags::hasSignal;
        }
        return false;
    }

    currency32_t removeRoadModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_RoadAndDirection roadAndDirection, CompanyId company, uint8_t roadType, uint8_t flags, World::Track::ModSection modSelection, uint8_t roadModObjIds)
    {
        currency32_t cost = 0;
        if (modSelection == Track::ModSection::single)
        {
            LocationOfInterest interest{ pos, roadAndDirection._data, company, roadType };
            removeRoadModToTrack(interest, flags, nullptr, modSelection, roadType, roadModObjIds, cost, company);
            return cost;
        }

        LocationOfInterestHashSet interestHashSet{};

        auto filterFunction = [flags, modSelection, roadType, roadModObjIds, &cost, company, &interestHashSet](const LocationOfInterest& interest) {
            return removeRoadModToTrack(interest, flags, &interestHashSet, modSelection, roadType, roadModObjIds, cost, company);
        };
        findAllRoadsFilterTransform(interestHashSet, TrackNetworkSearchFlags::none, pos, roadAndDirection, company, roadType, filterFunction, kNullTransformFunction);
        return cost;
    }
}
