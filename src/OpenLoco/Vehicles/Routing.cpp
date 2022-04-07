#include "Routing.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/AnimationManager.h"
#include "../Map/Map.hpp"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
#include "../Map/Track/Track.h"
#include "../Map/Track/TrackData.h"
#include "../ViewportManager.h"
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    using namespace OpenLoco::Interop;
    using namespace OpenLoco::Map;

    struct LocationOfInterest
    {
        Map::Pos3 loc;
        uint16_t trackAndDirection;
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

    struct LocationOfInterestHashMap
    {
        static constexpr auto kMapSize = 0x400;
        static constexpr auto kMapSizeMask = kMapSize - 1;
        static constexpr auto kMaxEntries = 0x39C;

    private:
#pragma pack(push, 1)
        struct ZAndTD
        {
            coord_t z;
            uint16_t trackAndDirection;
        };
        struct CAndT
        {
            CompanyId company;
            uint8_t trackType;
            uint8_t pad[0x2];
        };
#pragma pack(pop)

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
                while (_index < kMapSize && _map.locs[_index] == Map::Pos2{ -1, -1 })
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
#pragma pack(push, 1)
        Map::Pos2 locs[kMapSize];
        ZAndTD zAndTDs[kMapSize];
        CAndT cAndTs[kMapSize];
#pragma pack(pop)
        size_t count; // count does not need to be tightly packed as this is only accessed via a pointer to first 3 members

        LocationOfInterestHashMap()
            : zAndTDs{}
            , cAndTs{}
            , count()
        {
            std::fill(std::begin(locs), std::end(locs), Map::Pos2{ -1, -1 });
        }

        LocationOfInterest get(const uint16_t index) const
        {
            return LocationOfInterest{ Map::Pos3(locs[index].x, locs[index].y, zAndTDs[index].z), zAndTDs[index].trackAndDirection, cAndTs[index].company, cAndTs[index].trackType };
        }

        constexpr uint16_t hash(const LocationOfInterest& interest) const
        {
            return ((((interest.loc.x ^ interest.loc.z) / 32) ^ interest.loc.y) ^ interest.trackAndDirection) & kMapSizeMask;
        }

        // 0x004A38DE
        bool tryAdd(LocationOfInterest& interest)
        {
            auto index = hash(interest);
            for (; locs[index] != Map::Pos2{ -1, -1 }; ++index, index &= kMapSizeMask)
            {
                if (get(index) == interest)
                {
                    return false;
                }
            }
            if (count >= kMaxEntries)
            {
                return false;
            }
            locs[index] = interest.loc;
            zAndTDs[index] = ZAndTD{ interest.loc.z, interest.trackAndDirection };
            cAndTs[index] = CAndT{ interest.company, interest.trackType, {} };
            count++;
            return true;
        }

        Iterator begin() const
        {
            return Iterator(0, *this);
        }
        Iterator end() const
        {
            return Iterator(kMapSize, *this);
        }
    };

    using FilterFunction = bool (*)(const LocationOfInterest& interest);
    using TransformFunction = void (*)(const LocationOfInterestHashMap& hashMap);

    // Reverse direction map?
    static loco_global<uint8_t[16], 0x00503CAC> _503CAC;
    static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;
    static loco_global<FilterFunction, 0x01135F0E> _filterFunction;
    static loco_global<uint32_t, 0x01135F0A> _1135F0A;
    static loco_global<uint16_t, 0x01135FA6> _1135FA6;
    static loco_global<TransformFunction, 0x01135F12> _transformFunction;
    static loco_global<uint8_t, 0x01136085> _1136085;
    static loco_global<LocationOfInterestHashMap*, 0x01135F06> _1135F06;
    static loco_global<uint8_t[2], 0x0113601A> _113601A;
    static loco_global<uint16_t, 0x001135F88> _routingTransformData;

    static std::optional<std::pair<Map::SignalElement*, Map::TrackElement*>> findSignalOnTrack(const Map::Pos3& signalLoc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, const uint8_t index)
    {
        auto tile = Map::TileManager::get(signalLoc);
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
    uint8_t getSignalState(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags)
    {
        auto trackStart = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = Map::TrackData::getUnkTrack(trackAndDirection._data);
            trackStart += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
            flags ^= (1ULL << 31);
        }

        auto& trackPieces = Map::TrackData::getTrackPiece(trackAndDirection.id());
        auto& trackPiece = trackPieces[0];

        auto signalLoc = trackStart + Map::Pos3{ Math::Vector::rotate(Map::Pos2{ trackPiece.x, trackPiece.y }, trackAndDirection.cardinalDirection()), 0 };
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
    void setSignalState(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags)
    {
        const auto unk1 = flags & 0xFFFF;
        assert(unk1 != 10); // Only happens if wrong function was called call getSignalState
        auto trackStart = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = Map::TrackData::getUnkTrack(trackAndDirection._data);
            trackStart += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
            flags ^= (1ULL << 31);
        }

        auto& trackPieces = Map::TrackData::getTrackPiece(trackAndDirection.id());
        for (auto& trackPiece : trackPieces)
        {
            auto signalLoc = trackStart + Map::Pos3{ Math::Vector::rotate(Map::Pos2{ trackPiece.x, trackPiece.y }, trackAndDirection.cardinalDirection()), 0 };
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
                    Map::AnimationManager::createAnimation(0, signalLoc, signalLoc.z / 4);
                    Ui::ViewportManager::invalidate(signalLoc, signalLoc.z, signalLoc.z + 24, ZoomLevel::half);
                }
            }
        }
    }

    // 0x004A2AF0
    // Passes occupied state via _routingTransformData
    // Returns true for signals
    static bool findSignalsAndOccupation(const LocationOfInterest& interest)
    {
        auto nextLoc = interest.loc;
        const auto tad = interest.tad();
        auto& trackSize = Map::TrackData::getUnkTrack(tad._data);
        nextLoc += trackSize.pos;
        if (trackSize.rotationEnd < 12)
        {
            nextLoc -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
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

                if (vehicle->getFlags38() & (Vehicles::Flags38::unk_0 | Vehicles::Flags38::unk_2))
                {
                    continue;
                }

                if (vehicle->getTrackLoc() == interest.loc && vehicle->getVar2C().track == tad)
                {
                    _routingTransformData = 1;
                    break;
                }

                if (vehicle->getTrackLoc() == nextLoc && vehicle->getVar2C().track == backwardTaD)
                {
                    _routingTransformData = 1;
                    break;
                }
            }
        }
        return interest.trackAndDirection & (1 << 15);
    }

    // 0x004A2CE7
    // Passes occupied state via _routingTransformData
    static void setSignalsOccupiedState(const LocationOfInterestHashMap& hashMap)
    {
        for (const auto& interest : hashMap)
        {
            if (!(interest.trackAndDirection & (1 << 15)))
            {
                continue;
            }

            bool isOccupied = _routingTransformData & 1;
            uint32_t flags = (1ULL << 31) | (isOccupied ? 8ULL : 9ULL);
            setSignalState(interest.loc, interest.tad(), interest.trackType, flags);
        }
    }

    static void findAllUsableTrackInBlock(const LocationOfInterest& initialInterest, const FilterFunction filterFunction, LocationOfInterestHashMap& hashMap);

    // 0x004A313B
    // Iterates all individual tiles of a track piece to find tracks that need inspection
    static void findAllUsableTrackPieces(const LocationOfInterest& interest, const FilterFunction filterFunction, LocationOfInterestHashMap& hashMap)
    {
        if (!(_1135FA6 & (1 << 2)))
        {
            findAllUsableTrackInBlock(interest, filterFunction, hashMap);
            return;
        }

        const auto tad = interest.tad();
        auto nextLoc = interest.loc;
        if (tad.isReversed())
        {
            auto& trackSize = Map::TrackData::getUnkTrack(tad._data);
            nextLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextLoc -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
        }

        std::vector<LocationOfInterest> trackToCheck;

        for (auto& piece : Map::TrackData::getTrackPiece(tad.id()))
        {
            const auto connectFlags = piece.connectFlags[tad.cardinalDirection()];
            const auto pieceLoc = nextLoc + Map::Pos3{ Math::Vector::rotate(Map::Pos2{ piece.x, piece.y }, tad.cardinalDirection()), piece.z };
            auto tile = Map::TileManager::get(pieceLoc);
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

                const auto& targetPiece = Map::TrackData::getTrackPiece(elTrack->trackId())[elTrack->sequenceIndex()];
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

                const auto startTargetPos2 = Map::Pos2{ pieceLoc } - Math::Vector::rotate(Map::Pos2{ targetPiece.x, targetPiece.y }, elTrack->unkDirection());
                const auto startTargetPos = Map::Pos3{ startTargetPos2, static_cast<int16_t>(elTrack->baseZ() * 4 - targetPiece.z) };
                TrackAndDirection::_TrackAndDirection tad2(elTrack->trackId(), elTrack->unkDirection());
                LocationOfInterest newInterest{ startTargetPos, tad2._data, elTrack->owner(), elTrack->trackObjectId() };

                if (hashMap.tryAdd(newInterest))
                {
                    if (!filterFunction(newInterest))
                    {
                        trackToCheck.push_back(newInterest);
                    }
                }

                auto& trackSize = Map::TrackData::getUnkTrack(tad2._data);
                auto endTargetPos = startTargetPos + trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    endTargetPos -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
                }

                tad2.setReversed(!tad2.isReversed());
                LocationOfInterest newInterestR{ endTargetPos, tad2._data, elTrack->owner(), elTrack->trackObjectId() };

                if (hashMap.tryAdd(newInterestR))
                {
                    if (!filterFunction(newInterestR))
                    {
                        trackToCheck.push_back(newInterestR);
                    }
                }
            }
        }

        findAllUsableTrackInBlock(interest, filterFunction, hashMap);

        for (auto& interest2 : trackToCheck)
        {
            findAllUsableTrackPieces(interest2, filterFunction, hashMap);
        }
    }

    // 0x004A2FE6
    static void findAllUsableTrackInBlock(const LocationOfInterest& initialInterest, const FilterFunction filterFunction, LocationOfInterestHashMap& hashMap)
    {
        Map::Track::TrackConnections connections{};
        _113601A[0] = 0;
        _113601A[1] = 0;
        connections.size = 0;
        std::vector<LocationOfInterest> trackToCheck;

        const auto [trackEndLoc, trackEndRotation] = Map::Track::getTrackConnectionEnd(initialInterest.loc, initialInterest.tad()._data);
        Map::Track::getTrackConnections(trackEndLoc, trackEndRotation, connections, initialInterest.company, initialInterest.trackType);

        if (connections.size != 0)
        {
            for (size_t i = 0; i < connections.size; ++i)
            {
                uint16_t trackAndDirection2 = connections.data[i] & 0x81FF;
                LocationOfInterest interest{ trackEndLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (hashMap.tryAdd(interest))
                {
                    if (!filterFunction(interest))
                    {
                        trackToCheck.push_back(interest);
                    }
                }
            }
        }
        else
        {
            _1136085 = *_1136085 | (1 << 0);
        }

        if (!(_1135FA6 & (1 << 1)))
        {
            // odd logic here clearing a flag in a branch that can never hit
            auto nextLoc = initialInterest.loc;
            auto& trackSize = Map::TrackData::getUnkTrack(initialInterest.tad()._data);
            nextLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextLoc -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }

            connections.size = 0;
            const auto rotation = _503CAC[trackSize.rotationEnd];
            Map::Track::getTrackConnections(nextLoc, rotation, connections, initialInterest.company, initialInterest.trackType);
            for (size_t i = 0; i < connections.size; ++i)
            {
                uint16_t trackAndDirection2 = connections.data[i] & 0x81FF;
                LocationOfInterest interest{ nextLoc, trackAndDirection2, initialInterest.company, initialInterest.trackType };
                if (hashMap.tryAdd(interest))
                {
                    if (!filterFunction(interest))
                    {
                        trackToCheck.push_back(interest);
                    }
                }
            }
        }

        for (auto& interest : trackToCheck)
        {
            findAllUsableTrackPieces(interest, filterFunction, hashMap);
        }
    }

    // 0x004A2E46
    static void findAllTracksFilterTransform(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType, const FilterFunction filterFunction, const TransformFunction transformFunction)
    {
        _filterFunction = filterFunction;
        _transformFunction = transformFunction;
        LocationOfInterestHashMap interestMap{};
        _1135F06 = &interestMap;
        _1135F0A = 0;
        _1135FA6 = 5; // flags
        findAllUsableTrackInBlock(LocationOfInterest{ loc, trackAndDirection._data, company, trackType }, filterFunction, interestMap);
        if (reinterpret_cast<uint32_t>(transformFunction) != 0xFFFFFFFF)
        {
            transformFunction(interestMap);
        }
    }

    // 0x004A2AD7
    void sub_4A2AD7(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        _routingTransformData = 0;
        findAllTracksFilterTransform(loc, trackAndDirection, company, trackType, findSignalsAndOccupation, setSignalsOccupiedState);
    }

}
