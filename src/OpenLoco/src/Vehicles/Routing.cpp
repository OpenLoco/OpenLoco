#include "Routing.h"
#include "Economy/Economy.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Map/AnimationManager.h"
#include "Map/SignalElement.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Vehicle.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::Vehicles
{
    using namespace OpenLoco::Interop;
    using namespace OpenLoco::World;

    enum class TrackNetworkSearchFlags : uint16_t
    {
        none = 0,

        unk0 = 1U << 0,
        unk1 = 1U << 1,
        unk2 = 1U << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TrackNetworkSearchFlags);

    struct LocationOfInterest
    {
        World::Pos3 loc;
        uint16_t trackAndDirection; // This is a TaD with a AdditionalTaDFlags::hasSignal bit
        CompanyId company;
        uint8_t trackType;

        bool operator==(const LocationOfInterest& rhs)
        {
            return (loc == rhs.loc) && (trackAndDirection == rhs.trackAndDirection) && (company == rhs.company) && (trackType == rhs.trackType);
        }

        bool operator!=(const LocationOfInterest& rhs)
        {
            return !(*this == rhs);
        }

        TrackAndDirection::_TrackAndDirection tad() const
        {
            return TrackAndDirection::_TrackAndDirection((trackAndDirection & 0x1F8) >> 3, trackAndDirection & 0x7);
        }
    };

    // Note: This is not binary identical to vanilla so cannot be hooked!
    struct LocationOfInterestHashMap
    {
        static constexpr auto kMinFreeSlots = 100; // There must always be at least 100 free slots otherwise the hashmap gets very inefficient

    private:
        class Iterator
        {
            uint16_t _index;
            const LocationOfInterestHashMap& _map;

        public:
            Iterator(uint16_t index, const LocationOfInterestHashMap& map)
                : _index(index)
                , _map(map)
            {
                findAllocatedEntry();
            }

            void findAllocatedEntry()
            {
                while (_index < _map.mapSize && _map.locs[_index].loc == World::Pos3{ -1, -1, 0 })
                {
                    _index++;
                }
            }

            Iterator& operator++()
            {
                _index++;
                findAllocatedEntry();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator retval = *this;
                ++(*this);
                return retval;
            }

            bool operator==(Iterator& other) const
            {
                return _index == other._index;
            }
            bool operator!=(Iterator& other) const
            {
                return !(*this == other);
            }

            LocationOfInterest operator*() const
            {
                return _map.get(_index);
            }
            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = LocationOfInterest;
            using pointer = LocationOfInterest*;
            using reference = LocationOfInterest&;
            using iterator_category = std::forward_iterator_tag;
        };

    public:
        std::vector<LocationOfInterest> locs;
        size_t count;
        uint16_t mapSize;
        uint16_t mapSizeMask;
        uint16_t maxEntries;

        LocationOfInterestHashMap(uint16_t _maxSize)
            : count()
            , mapSize(_maxSize)
            , mapSizeMask(_maxSize - 1)
            , maxEntries(_maxSize - kMinFreeSlots)
        {
            assert((mapSize & (mapSizeMask)) == 0); // Only works with powers of 2
            locs.resize(mapSize);
            std::fill(std::begin(locs), std::end(locs), LocationOfInterest{ World::Pos3{ -1, -1, 0 }, 0, CompanyId::null, 0 });
        }

        LocationOfInterest get(const uint16_t index) const
        {
            return locs[index];
        }

        constexpr uint16_t hash(const LocationOfInterest& interest) const
        {
            return ((((interest.loc.x ^ interest.loc.z) / 32) ^ interest.loc.y) ^ interest.trackAndDirection) & mapSizeMask;
        }

        // 0x004A38DE & 0x004A3972
        bool tryAdd(LocationOfInterest& interest)
        {
            auto index = hash(interest);
            for (; locs[index].loc != World::Pos3{ -1, -1, 0 }; ++index, index &= mapSizeMask)
            {
                if (get(index) == interest)
                {
                    return false;
                }
            }
            if (count >= maxEntries)
            {
                return false;
            }
            locs[index] = interest;
            count++;
            return true;
        }

        Iterator begin() const
        {
            return Iterator(0, *this);
        }
        Iterator end() const
        {
            return Iterator(mapSize, *this);
        }
    };

    // using FilterFunction = bool (*)(const LocationOfInterest& interest);          // TODO C++20 make these concepts
    // using TransformFunction = void (*)(const LocationOfInterestHashMap& hashMap); // TODO C++20 make these concepts

    constexpr auto kNullTransformFunction = [](const LocationOfInterestHashMap&) {};

    // static loco_global<FilterFunction, 0x01135F0E> _filterFunction; // Note: No longer passed by global
    // static loco_global<uint32_t, 0x01135F0A> _hashMapSize;          // Note: No longer passed by global
    // static loco_global<TransformFunction, 0x01135F12> _transformFunction; // Note: No longer passed by global
    // static loco_global<LocationOfInterestHashMap*, 0x01135F06> _1135F06; // Note: No longer binary identical so never set this
    // static loco_global<uint16_t, 0x001135F88> _routingTransformData; // Note: No longer passed by global

    static loco_global<TrackNetworkSearchFlags, 0x01135FA6> _findTrackNetworkFlags;
    static loco_global<uint8_t, 0x01136085> _1136085;
    static loco_global<uint8_t[2], 0x0113601A> _113601A; // Track Connection mod global

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

            if (elTrack->unkDirection() != trackAndDirection.cardinalDirection())
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
    uint8_t getSignalState(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags)
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
            return 0;
        }

        auto* elSignal = res->first;

        // edx
        auto& signalSide = (flags & (1ULL << 31)) ? elSignal->getRight() : elSignal->getLeft();
        uint8_t ret = 0;
        if (signalSide.hasUnk4_40())
        {
            ret |= (1 << 0);
        }
        if (signalSide.hasSignal())
        {
            ret |= (1 << 1);
        }
        if (flags & (1ULL << 31))
        {
            if (!elSignal->getLeft().hasSignal() || elSignal->isLeftGhost())
            {
                ret |= (1 << 2);
            }

            if (elSignal->isRightGhost() && elSignal->getLeft().hasSignal())
            {
                ret |= (1 << 1);
            }
        }
        else
        {
            if (!elSignal->getRight().hasSignal() || elSignal->isRightGhost())
            {
                ret |= (1 << 2);
            }

            if (elSignal->isLeftGhost() && elSignal->getRight().hasSignal())
            {
                ret |= (1 << 1);
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
                signalSide.setUnk4_40(true);
            }
            else if (unk1 > 8)
            {
                signalSide.setUnk4_40(false);
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
    // Passes occupied state via _routingTransformData
    // Returns true for signal block end
    static bool findOccupationByBlock(const LocationOfInterest& interest, uint16_t& routingTransformData)
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
                    routingTransformData = 1;
                    break;
                }

                if (vehicle->getTrackLoc() == nextLoc && vehicle->getTrackAndDirection().track == backwardTaD)
                {
                    routingTransformData = 1;
                    break;
                }
            }
        }
        return interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal;
    }

    // 0x004A2CE7
    static void setSignalsOccupiedState(const LocationOfInterestHashMap& hashMap, const uint16_t& routingTransformData)
    {
        for (const auto& interest : hashMap)
        {
            if (!(interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal))
            {
                continue;
            }

            bool isOccupied = routingTransformData & 1;
            uint32_t flags = (1ULL << 31) | (isOccupied ? 8ULL : 9ULL);
            setSignalState(interest.loc, interest.tad(), interest.trackType, flags);
        }
    }

    // 0x004A2D4C
    static bool sub_4A2D4C(const LocationOfInterest& interest, uint16_t& unk)
    {
        if (!(interest.trackAndDirection & World::Track::AdditionalTaDFlags::hasSignal))
        {
            return false;
        }

        if (getSignalState(interest.loc, interest.tad(), interest.trackType, (1ULL << 31)) & (1 << 0))
        {
            unk |= (1 << 0);
        }
        else
        {
            unk |= (1 << 1);
        }
        return true;
    }

    template<typename FilterFunction>
    static void findAllUsableTrackInNetwork(std::vector<LocationOfInterest>& additionalTrackToCheck, const LocationOfInterest& initialInterest, FilterFunction&& filterFunction, LocationOfInterestHashMap& hashMap);

    // 0x004A313B & 0x004A35B7
    // Iterates all individual tiles of a track piece to find tracks that need inspection
    template<typename FilterFunction>
    static void findAllUsableTrackPieces(std::vector<LocationOfInterest>& additionalTrackToCheck, const LocationOfInterest& interest, FilterFunction&& filterFunction, LocationOfInterestHashMap& hashMap)
    {
        if ((_findTrackNetworkFlags & TrackNetworkSearchFlags::unk2) == TrackNetworkSearchFlags::none)
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

                if (elTrack->isFlag5() || elTrack->isGhost())
                {
                    continue;
                }

                const auto& targetPiece = World::TrackData::getTrackPiece(elTrack->trackId())[elTrack->sequenceIndex()];
                const auto targetConnectFlags = targetPiece.connectFlags[elTrack->unkDirection()];
                if ((targetConnectFlags & connectFlags) == 0)
                {
                    continue;
                }

                // If identical then no need to keep checking
                if (elTrack->unkDirection() == tad.cardinalDirection()
                    && elTrack->sequenceIndex() == piece.index
                    && elTrack->trackObjectId() == interest.trackType
                    && elTrack->trackId() == tad.id())
                {
                    continue;
                }

                const auto startTargetPos2 = World::Pos2{ pieceLoc } - Math::Vector::rotate(World::Pos2{ targetPiece.x, targetPiece.y }, elTrack->unkDirection());
                const auto startTargetPos = World::Pos3{ startTargetPos2, static_cast<int16_t>(elTrack->baseHeight() - targetPiece.z) };
                TrackAndDirection::_TrackAndDirection tad2(elTrack->trackId(), elTrack->unkDirection());
                LocationOfInterest newInterest{ startTargetPos, tad2._data, elTrack->owner(), elTrack->trackObjectId() };

                if (hashMap.tryAdd(newInterest))
                {
                    if (!filterFunction(newInterest))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, newInterest, filterFunction, hashMap);
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

                if (hashMap.tryAdd(newInterestR))
                {
                    if (!filterFunction(newInterestR))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, newInterestR, filterFunction, hashMap);
                        additionalTrackToCheck.push_back(newInterestR);
                    }
                }
            }
        }
    }

    // 0x004A2FE6 & 0x004A3462
    template<typename FilterFunction>
    static void findAllUsableTrackInNetwork(std::vector<LocationOfInterest>& additionalTrackToCheck, const LocationOfInterest& initialInterest, FilterFunction&& filterFunction, LocationOfInterestHashMap& hashMap)
    {
        World::Track::TrackConnections connections{};
        _113601A[0] = 0;
        _113601A[1] = 0;
        connections.size = 0;

        const auto [trackEndLoc, trackEndRotation] = World::Track::getTrackConnectionEnd(initialInterest.loc, initialInterest.tad()._data);
        World::Track::getTrackConnections(trackEndLoc, trackEndRotation, connections, initialInterest.company, initialInterest.trackType);

        if (connections.size != 0)
        {
            for (size_t i = 0; i < connections.size; ++i)
            {
                uint16_t trackAndDirection2 = connections.data[i] & World::Track::AdditionalTaDFlags::basicTaDWithSignalMask;
                LocationOfInterest interest{ trackEndLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (hashMap.tryAdd(interest))
                {
                    if (!filterFunction(interest))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, interest, filterFunction, hashMap);
                        additionalTrackToCheck.push_back(interest);
                    }
                }
            }
        }
        else
        {
            _1136085 = *_1136085 | (1 << 0);
        }

        if ((_findTrackNetworkFlags & TrackNetworkSearchFlags::unk1) == TrackNetworkSearchFlags::none)
        {
            // odd logic here clearing a flag in a branch that can never hit
            auto nextLoc = initialInterest.loc;
            auto& trackSize = World::TrackData::getUnkTrack(initialInterest.tad()._data);
            nextLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextLoc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }

            connections.size = 0;
            const auto rotation = World::kReverseRotation[trackSize.rotationEnd];
            World::Track::getTrackConnections(nextLoc, rotation, connections, initialInterest.company, initialInterest.trackType);
            for (size_t i = 0; i < connections.size; ++i)
            {
                uint16_t trackAndDirection2 = connections.data[i] & World::Track::AdditionalTaDFlags::basicTaDWithSignalMask;
                LocationOfInterest interest{ nextLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (hashMap.tryAdd(interest))
                {
                    if (!filterFunction(interest))
                    {
                        findAllUsableTrackPieces(additionalTrackToCheck, interest, filterFunction, hashMap);
                        additionalTrackToCheck.push_back(interest);
                    }
                }
            }
        }
    }

    constexpr size_t kSignalHashMapSize = 0x400;
    constexpr size_t kTrackModHashMapSize = 0x1000;

    // 0x004A2E46 & 0x004A2DE4
    template<typename FilterFunction, typename TransformFunction>
    static void findAllTracksFilterTransform(LocationOfInterestHashMap& interestMap, TrackNetworkSearchFlags searchFlags, const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType, FilterFunction&& filterFunction, TransformFunction&& transformFunction)
    {
        // _1135F06 = &interestMap;
        // _filterFunction = filterFunction;
        // _transformFunction = transformFunction;
        // _1135F0A = 0;

        _findTrackNetworkFlags = searchFlags;

        // Note: This function and its call chain findAllUsableTrackInNetwork and findAllUsableTrackPieces have been modified
        // to not be recursive anymore.
        std::vector<LocationOfInterest> trackToCheck{ LocationOfInterest{ loc, trackAndDirection._data, company, trackType } };
        while (!trackToCheck.empty())
        {
            const auto interest = trackToCheck.back();
            trackToCheck.pop_back();
            findAllUsableTrackInNetwork(trackToCheck, interest, filterFunction, interestMap);
        }
        transformFunction(interestMap);
    }

    // 0x004A2AD7
    void sub_4A2AD7(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        // 0x001135F88
        uint16_t routingTransformData = 0;

        auto filterFunction = [&routingTransformData](const LocationOfInterest& interest) { return findOccupationByBlock(interest, routingTransformData); };
        auto transformFunction = [&routingTransformData](const LocationOfInterestHashMap& interestMap) { setSignalsOccupiedState(interestMap, routingTransformData); };
        LocationOfInterestHashMap interestMap{ kSignalHashMapSize };

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::unk0 | TrackNetworkSearchFlags::unk2,
            loc,
            trackAndDirection,
            company,
            trackType,
            filterFunction,
            transformFunction);
    }

    uint8_t sub_4A2A58(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        // 0x001135F88
        uint16_t unk = 0;
        LocationOfInterestHashMap interestMap{ kSignalHashMapSize };
        auto filterFunction = [&unk](const LocationOfInterest& interest) { return sub_4A2D4C(interest, unk); };

        findAllTracksFilterTransform(
            interestMap,
            TrackNetworkSearchFlags::unk0 | TrackNetworkSearchFlags::unk2,
            loc,
            trackAndDirection,
            company,
            trackType,
            filterFunction,
            kNullTransformFunction);

        return unk;
    }

    // 0x004A5D94
    static bool applyTrackModToTrack(const LocationOfInterest& interest, const uint8_t flags, LocationOfInterestHashMap* hashMap, uint8_t modSelection, uint8_t trackObjectId, uint8_t trackModObjectIds, currency32_t& totalCost, CompanyId companyId, bool& hasFailedAllPlacement)
    {
        // If not in single segment mode then we should add the reverse
        // direction of track to the hashmap to prevent it being visited.
        // This is because track mods do not have directions so applying
        // to the reverse would do nothing (or worse double spend)
        if (hashMap != nullptr)
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

            hashMap->tryAdd(reverseInterest);
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

            const auto pieceFlags = TrackData::getTrackCompatibleFlags(interest.tad().id());
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
                    if (elTrack->unkDirection() != tad.cardinalDirection())
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
                        const auto cost = (baseCost * TrackData::getTrackCostFactor(tad.id())) / 256;
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

        if (modSelection == 1)
        {
            return interest.trackAndDirection & Track::AdditionalTaDFlags::hasSignal;
        }
        return false;
    }

    // 0x004A6136
    static bool removeTrackModToTrack(const LocationOfInterest& interest, const uint8_t flags, LocationOfInterestHashMap* hashMap, uint8_t modSelection, uint8_t trackObjectId, uint8_t trackModObjectIds, currency32_t& totalCost, CompanyId companyId)
    {
        // If not in single segment mode then we should add the reverse
        // direction of track to the hashmap to prevent it being visited.
        // This is because track mods do not have directions so applying
        // to the reverse would do nothing (or worse double spend)
        if (hashMap != nullptr)
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

            hashMap->tryAdd(reverseInterest);
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
                if (elTrack->unkDirection() != tad.cardinalDirection())
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
                    const auto cost = (baseCost * TrackData::getTrackCostFactor(tad.id())) / 256;
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

        if (modSelection == 1)
        {
            return interest.trackAndDirection & Track::AdditionalTaDFlags::hasSignal;
        }
        return false;
    }

    ApplyTrackModsResult applyTrackModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_TrackAndDirection trackAndDirection, CompanyId company, uint8_t trackType, uint8_t flags, uint8_t modSelection, uint8_t trackModObjIds)
    {
        ApplyTrackModsResult result{};
        result.cost = 0;
        result.allPlacementsFailed = true;
        result.networkTooComplex = false;

        if (modSelection == 0)
        {
            LocationOfInterest interest{ pos, trackAndDirection._data, company, trackType };
            applyTrackModToTrack(interest, flags, nullptr, modSelection, trackType, trackModObjIds, result.cost, company, result.allPlacementsFailed);
            return result;
        }

        LocationOfInterestHashMap interestHashMap{ kTrackModHashMapSize };

        auto filterFunction = [flags, modSelection, trackType, trackModObjIds, &result, company, &interestHashMap](const LocationOfInterest& interest) {
            return applyTrackModToTrack(interest, flags, &interestHashMap, modSelection, trackType, trackModObjIds, result.cost, company, result.allPlacementsFailed);
        };
        findAllTracksFilterTransform(interestHashMap, TrackNetworkSearchFlags::unk0, pos, trackAndDirection, company, trackType, filterFunction, kNullTransformFunction);
        result.networkTooComplex = interestHashMap.count >= interestHashMap.maxEntries;
        return result;
    }

    currency32_t removeTrackModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_TrackAndDirection trackAndDirection, CompanyId company, uint8_t trackType, uint8_t flags, uint8_t modSelection, uint8_t trackModObjIds)
    {
        currency32_t cost = 0;
        if (modSelection == 0)
        {
            LocationOfInterest interest{ pos, trackAndDirection._data, company, trackType };
            removeTrackModToTrack(interest, flags, nullptr, modSelection, trackType, trackModObjIds, cost, company);
            return cost;
        }

        LocationOfInterestHashMap interestHashMap{ kTrackModHashMapSize };

        auto filterFunction = [flags, modSelection, trackType, trackModObjIds, &cost, company, &interestHashMap](const LocationOfInterest& interest) {
            return removeTrackModToTrack(interest, flags, &interestHashMap, modSelection, trackType, trackModObjIds, cost, company);
        };
        findAllTracksFilterTransform(interestHashMap, TrackNetworkSearchFlags::unk0, pos, trackAndDirection, company, trackType, filterFunction, kNullTransformFunction);
        return cost;
    }
}
