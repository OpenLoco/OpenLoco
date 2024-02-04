#include "Paint.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "PaintEntity.h"
#include "PaintTile.h"
#include "PaintTrack.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <iostream>
#include <map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    PaintSession _session;

    void PaintSession::setEntityPosition(const World::Pos2& pos)
    {
        _spritePositionX = pos.x;
        _spritePositionY = pos.y;
    }
    void PaintSession::setMapPosition(const World::Pos2& pos)
    {
        _mapPosition = pos;
    }
    void PaintSession::setUnkPosition(const World::Pos2& pos)
    {
        _unkPositionX = pos.x;
        _unkPositionY = pos.y;
    }
    void PaintSession::setVpPosition(const Ui::Point& pos)
    {
        _vpPositionX = pos.x;
        _vpPositionY = pos.y;
    }

    void PaintSession::resetTileColumn(const Ui::Point& pos)
    {
        setVpPosition(pos);
        _didPassSurface = false;
        setBridgeEntry(kNullBridgeEntry);
        _525CF8 = SegmentFlags::none;
        _trackRoadAdditionSupports = TrackRoadAdditionSupports{};
        std::fill(std::begin(_trackRoadPaintStructs), std::end(_trackRoadPaintStructs), nullptr);
        std::fill(std::begin(_trackRoadAdditionsPaintStructs), std::end(_trackRoadAdditionsPaintStructs), nullptr);
        _112C300 = 0;
        _112C306 = 0;
    }

    void PaintSession::setMaxHeight(const World::Pos2& loc)
    {
        auto tile = World::TileManager::get(loc);
        uint8_t maxClearZ = 0;
        for (const auto& el : tile)
        {
            maxClearZ = std::max(maxClearZ, el.clearZ());
            const auto* surface = el.as<World::SurfaceElement>();
            if (!surface)
            {
                continue;
            }

            if (surface->water())
            {
                maxClearZ = std::max<uint8_t>(maxClearZ, surface->water() * World::kMicroToSmallZStep);
            }
            if (surface->isIndustrial())
            {
                maxClearZ = std::max<uint8_t>(maxClearZ, surface->clearZ() + 24);
            }
        }
        _maxHeight = (maxClearZ * World::kSmallZStep) + 32;
    }

    loco_global<int32_t[4], 0x4FD120> _addToStringPlotList;
    loco_global<int32_t[4], 0x4FD130> _4FD130;
    loco_global<int32_t[4], 0x4FD140> _4FD140;
    loco_global<int32_t[4], 0x4FD150> _4FD150;
    loco_global<int32_t[4], 0x4FD1E0> _4FD1E0;
    loco_global<int32_t[4], 0x4FD170> _4FD170;
    loco_global<int32_t[4], 0x4FD180> _4FD180;
    loco_global<int32_t[4], 0x4FD200> _4FD200;

    /*
     * Flips the X axis so 1 and 3 are swapped 0 and 2 will stay the same.
     * { 0, 3, 2, 1 }
     */
    inline constexpr uint8_t directionFlipXAxis(const uint8_t direction)
    {
        return (direction * 3) % 4;
    }

    static int32_t remapPositionToQuadrant(const PaintStruct& ps, uint8_t rotation)
    {
        constexpr auto mapRangeMax = kMaxPaintQuadrants * World::kTileSize;
        constexpr auto mapRangeCenter = mapRangeMax / 2;

        const auto x = ps.bounds.x;
        const auto y = ps.bounds.y;
        // NOTE: We are not calling CoordsXY::Rotate on purpose to mix in the additional
        // value without a secondary switch.
        switch (rotation & 3)
        {
            case 0:
                return x + y;
            case 1:
                // Because one component may be the maximum we add the center to be a positive value.
                return (y - x) + mapRangeCenter;
            case 2:
                // If both components would be the maximum it would be the negative xy, to be positive add max.
                return (-(y + x)) + mapRangeMax;
            case 3:
                // Same as 1 but inverted.
                return (x - y) + mapRangeCenter;
        }
        return 0;
    }

    void PaintSession::addPSToQuadrant(PaintStruct& ps)
    {
        const auto positionHash = remapPositionToQuadrant(ps, currentRotation);

        // Values below zero or above MaxPaintQuadrants are void, corners also share the same quadrant as void.
        const uint32_t paintQuadrantIndex = std::clamp(positionHash / World::kTileSize, 0, kMaxPaintQuadrants - 1);

        ps.quadrantIndex = paintQuadrantIndex;
        ps.nextQuadrantPS = _quadrants[paintQuadrantIndex];
        _quadrants[paintQuadrantIndex] = &ps;

        _quadrantBackIndex = std::min(*_quadrantBackIndex, paintQuadrantIndex);
        _quadrantFrontIndex = std::max(*_quadrantFrontIndex, paintQuadrantIndex);
    }

    // 0x004FD120
    PaintStringStruct* PaintSession::addToStringPlotList(const uint32_t amount, const StringId stringId, const uint16_t z, const int16_t xOffset, const int8_t* yOffsets, const uint16_t colour)
    {
        auto* psString = allocatePaintStruct<PaintStringStruct>();
        if (psString == nullptr)
        {
            return nullptr;
        }
        psString->stringId = stringId;
        psString->next = nullptr;
        std::memcpy(&psString->args[0], &amount, sizeof(amount));
        psString->yOffsets = yOffsets;
        psString->colour = colour;

        const auto& vpPos = World::gameToScreen(World::Pos3(getSpritePosition(), z), currentRotation);
        psString->vpPos.x = vpPos.x + xOffset;
        psString->vpPos.y = vpPos.y;

        attachStringStruct(*psString);
        return psString;
    }

    // 0x004FD130
    PaintStruct* PaintSession::addToPlotListAsParent(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxSize)
    {
        return addToPlotListAsParent(imageId, offset, offset, boundBoxSize);
    }

    static constexpr bool imageWithinRT(const Ui::viewport_pos& imagePos, const Gfx::G1Element& g1, const Gfx::RenderTarget& rt)
    {
        int32_t left = imagePos.x + g1.xOffset;
        int32_t bottom = imagePos.y + g1.yOffset;

        int32_t right = left + g1.width;
        int32_t top = bottom + g1.height;

        if (right <= rt.x)
            return false;
        if (top <= rt.y)
            return false;
        if (left >= rt.x + rt.width)
            return false;
        if (bottom >= rt.y + rt.height)
            return false;
        return true;
    }

    static const std::array<std::string, 44> _trackIdNames = {
        "Straight",
        "Diagonal",
        "LeftCurveVerySmall",
        "RightCurveVerySmall",
        "LeftCurveSmall",
        "RightCurveSmall",
        "LeftCurve",
        "RightCurve",
        "LeftCurveLarge",
        "RightCurveLarge",
        "DiagonalLeftCurveLarge",
        "DiagonalRightCurveLarge",
        "SBendLeft",
        "SBendRight",
        "StraightSlopeUp",
        "StraightSlopeDown",
        "StraightSteepSlopeUp",
        "StraightSteepSlopeDown",
        "LeftCurveSmallSlopeUp",
        "RightCurveSmallSlopeUp",
        "LeftCurveSmallSlopeDown",
        "RightCurveSmallSlopeDown",
        "LeftCurveSmallSteepSlopeUp",
        "RightCurveSmallSteepSlopeUp",
        "LeftCurveSmallSteepSlopeDown",
        "RightCurveSmallSteepSlopeDown",
        "unkStraight1",
        "unkStraight2",
        "unkLeftCurveVerySmall1",
        "unkLeftCurveVerySmall2",
        "unkRightCurveVerySmall1",
        "unkRightCurveVerySmall2",
        "unkSBendRight",
        "unkSBendLeft",
        "unkStraightSteepSlopeUp1",
        "unkStraightSteepSlopeUp2",
        "unkStraightSteepSlopeDown1",
        "unkStraightSteepSlopeDown2",
        "sBendToDualTrack",
        "sBendToSingleTrack",
        "unkSBendToDualTrack",
        "unkSBendToSingleTrack",
        "turnaround",
        "unkTurnaround",
    };

    std::array<std::string, 9> kSegmentFlagNames = {
        "x0y0", //= 1U << 0, // 0: (x:0, y:0)
        "x2y0", //= 1U << 1, // 2: (x:2, y:0)
        "x0y2", //= 1U << 2, // 6: (x:0, y:2)
        "x2y2", //= 1U << 3, // 8: (x:2, y:2)
        "x1y1", //= 1U << 4, // 4: (x:1, y:1)
        "x1y0", //= 1U << 5, // 1: (x:1, y:0)
        "x0y1", //= 1U << 6, // 3: (x:0, y:1)
        "x2y1", //= 1U << 7, // 5: (x:2, y:1)
        "x1y2", //= 1U << 8, // 7: (x:1, y:2)
    };

    std::string getSegmentFlagsString(uint16_t flags)
    {
        std::string result;
        for (auto i = 0U; i < 9; ++i)
        {
            if (flags & (1 << i))
            {
                if (!result.empty())
                {
                    result += " | ";
                }
                result += fmt::format("SegmentFlags::{}", kSegmentFlagNames[i]);
            }
        }
        return result;
    }

    constexpr uint8_t rotl4bit(uint8_t val, uint8_t rotation)
    {
        return ((val << rotation) | (val >> (4 - rotation))) & 0xF;
    }

    constexpr uint8_t rotr4bit(uint8_t val, uint8_t rotation)
    {
        return ((val >> rotation) | (val << (4 - rotation))) & 0xF;
    }

    constexpr std::array<std::array<uint8_t, 4>, 4> kSegMap1 = {
        std::array<uint8_t, 4>{ 0, 1, 2, 3 },
        std::array<uint8_t, 4>{ 2, 0, 3, 1 },
        std::array<uint8_t, 4>{ 3, 2, 1, 0 },
        std::array<uint8_t, 4>{ 1, 3, 0, 2 },
    };
    constexpr std::array<std::array<uint8_t, 4>, 4> kSegMap2 = {
        std::array<uint8_t, 4>{ 0, 1, 2, 3 },
        std::array<uint8_t, 4>{ 1, 3, 0, 2 },
        std::array<uint8_t, 4>{ 3, 2, 1, 0 },
        std::array<uint8_t, 4>{ 2, 0, 3, 1 },
    };

    constexpr SegmentFlags rotlSegmentFlags(SegmentFlags val, uint8_t rotation)
    {
        SegmentFlags ret = SegmentFlags::none;
        const auto _val = enumValue(val);
        for (auto i = 0U; i < 4; ++i)
        {
            if (_val & (1U << i))
            {
                ret |= static_cast<SegmentFlags>(1U << kSegMap1[rotation][i]);
            }
        }
        for (auto i = 5U; i < 9; ++i)
        {
            if (_val & (1U << i))
            {
                ret |= static_cast<SegmentFlags>(1U << (kSegMap2[rotation][i - 5] + 5));
            }
        }

        return ret | (val & SegmentFlags::x1y1);
    }

    struct OffsetFunc
    {
        uint8_t trackId;
        uint8_t sequence;
        uint8_t rotation;
    };

    std::map<uint32_t, std::string> _usedImages;
    std::array<std::map<uint32_t, std::string>, 2> _usedImagesA;
    std::map<uint32_t, OffsetFunc> _outputedOffsets;
    std::array<std::map<uint32_t, OffsetFunc>, 2> _outputedOffsetsA;

    const std::array<std::string, 4> kPriorityStrs = { "", "Ballast", "Sleeper", "Rail" };
    const std::array<std::string, 4> kRotationNames = { "NE", "SE", "SW", "NW" };

    std::string getDefaultTrackImageName(uint8_t trackId, uint8_t index, int8_t priority, uint8_t rotation)
    {
        return fmt::format("k{}{}{}{}", _trackIdNames[trackId], index, kPriorityStrs[priority], kRotationNames[rotation]);
    }
    std::string getDefaultTrackAdditionImageName(uint8_t trackId, uint8_t index, uint8_t rotation)
    {
        return fmt::format("k{}{}{}", _trackIdNames[trackId], index, kRotationNames[rotation]);
    }
    std::string getDefaultTrackAdditionSupportImageName(uint8_t trackId, uint8_t index, uint8_t rotation)
    {
        return fmt::format("k{}{}Support{}", _trackIdNames[trackId], index, kRotationNames[rotation]);
    }
    std::string getDefaultTrackAdditionSupportConnectorImageName(uint8_t trackId, uint8_t index, uint8_t rotation)
    {
        return fmt::format("k{}{}SupportConnector{}", _trackIdNames[trackId], index, kRotationNames[rotation]);
    }

    std::array<std::string, 3> rotationTablesNames = { "kRotationTable2301", "kRotationTable3012", "kRotationTable1230" };

    std::pair<std::string, bool> printCommonPreamble(const std::array<uint32_t, 4>& callOffsets, uint8_t trackId, uint8_t index)
    {
        uint32_t seenCount = 0;
        uint8_t r = 0;
        for (auto& co : callOffsets)
        {
            if (_outputedOffsets.count(co) != 0)
            {
                seenCount++;
            }
            else
            {
                _outputedOffsets.insert(std::make_pair(co, OffsetFunc{ trackId, index, r }));
            }
            r++;
        }
        std::string ppName = fmt::format("k{}{}", _trackIdNames[trackId], index);

        if (seenCount == 2)
        {
            std::cout << "// Already seen " << seenCount << "\n";
        }
        else if (seenCount == 4)
        {
            uint8_t mirrorTrackId = 0;
            uint8_t mirrorIndex = 0;
            std::array<uint8_t, 4> rotationTable = {};
            for (auto i = 0; i < 4; ++i)
            {
                auto& f = _outputedOffsets[callOffsets[i]];
                if (i != 0)
                {
                    assert(mirrorIndex == f.sequence);
                    assert(mirrorTrackId == f.trackId);
                }
                rotationTable[i] = f.rotation;
                mirrorIndex = f.sequence;
                mirrorTrackId = f.trackId;
            }

            std::string mirrorPP = fmt::format("k{}{}", _trackIdNames[mirrorTrackId], mirrorIndex);
            std::string rotationTableName = "";
            if (rotationTable[0] == 2 && rotationTable[1] == 3 && rotationTable[2] == 0 && rotationTable[3] == 1)
            {
                rotationTableName = rotationTablesNames[0];
            }
            else if (rotationTable[0] == 3 && rotationTable[1] == 0 && rotationTable[2] == 1 && rotationTable[3] == 2)
            {
                rotationTableName = rotationTablesNames[1];
            }
            else if (rotationTable[0] == 1 && rotationTable[1] == 2 && rotationTable[2] == 3 && rotationTable[3] == 0)
            {
                rotationTableName = rotationTablesNames[2];
            }
            else
            {
                assert(false);
            }
            // assert(rotationTable[0] == 2);
            // assert(rotationTable[1] == 3);
            // assert(rotationTable[2] == 0);
            // assert(rotationTable[3] == 1);

            std::cout << fmt::format("constexpr TrackPaintPiece {} = rotateTrackPP({}, {});\n\n", ppName, mirrorPP, rotationTableName);
            return std::make_pair(ppName, false);
        }
        std::cout << fmt::format("// 0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}\n", callOffsets[0], callOffsets[1], callOffsets[2], callOffsets[3]);
        std::cout << fmt::format("constexpr TrackPaintPiece {} = {{\n", ppName);
        return std::make_pair(ppName, true);
    }

    void printCommonAlt(const std::array<World::Pos3, 4>& boundingBoxOffsets, const std::array<World::Pos3, 4>& boundingBoxSizes, const std::array<uint8_t, 4>& bridgeEdges, const std::array<uint8_t, 4>& bridgeQuarters, const std::array<uint8_t, 4>& bridgeType, const std::array<int8_t, 4>& tunnelHeights, const std::array<SegmentFlags, 4>& segments)
    {
        std::cout << "    /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{\n";
        for (auto& bbO : boundingBoxOffsets)
        {
            std::cout << fmt::format("        World::Pos3{{ {}, {}, {} }},\n", bbO.x, bbO.y, bbO.z);
        }
        std::cout << "    },\n";

        std::cout << "    /* BoundingBoxSizes */ std::array<World::Pos3, 4>{\n";
        for (auto& bbS : boundingBoxSizes)
        {
            std::cout << fmt::format("        World::Pos3{{ {}, {}, {} }},\n", bbS.x, bbS.y, bbS.z);
        }
        std::cout << "    },\n";

        // std::cout << "    /* BridgeEdges */ std::array<uint8_t, 4>{\n";
        // std::cout << fmt::format("        {:#06b}, {:#06b}, {:#06b}, {:#06b},\n", bridgeEdges[0], bridgeEdges[1], bridgeEdges[2], bridgeEdges[3]);
        // std::cout << "    },\n";
        std::cout << fmt::format("    /* BridgeEdges */ {:#06b},\n", bridgeEdges[0]);

        assert(bridgeEdges[1] == rotl4bit(bridgeEdges[0], 1));
        assert(bridgeEdges[2] == rotl4bit(bridgeEdges[0], 2));
        assert(bridgeEdges[3] == rotl4bit(bridgeEdges[0], 3));

        // std::cout << "    /* BridgeQuarters */ std::array<uint8_t, 4>{\n";
        // std::cout << fmt::format("        {:#06b}, {:#06b}, {:#06b}, {:#06b},\n", bridgeQuarters[0], bridgeQuarters[1], bridgeQuarters[2], bridgeQuarters[3]);
        // std::cout << "    },\n";
        std::cout << fmt::format("    /* BridgeQuarters */ {:#06b},\n", bridgeQuarters[0]);

        assert(bridgeQuarters[1] == rotl4bit(bridgeQuarters[0], 1));
        assert(bridgeQuarters[2] == rotl4bit(bridgeQuarters[0], 2));
        assert(bridgeQuarters[3] == rotl4bit(bridgeQuarters[0], 3));

        if ((bridgeType[0] == 0) && (bridgeType[1] == 0) && (bridgeType[2] == 0) && (bridgeType[3] == 0))
        {
            std::cout << fmt::format("    /* BridgeType */ kFlatBridge,\n");
        }
        else
        {
            std::cout << "    /* BridgeType */ std::array<uint8_t, 4>{\n";
            std::cout << "        ";
            for (auto bt : bridgeType)
            {
                std::cout << fmt::format("{}, ", bt);
            }
            std::cout << "\n";
            std::cout << "    },\n";
        }

        if ((tunnelHeights[0] == -1) && (tunnelHeights[1] == -1) && (tunnelHeights[2] == -1) && (tunnelHeights[3] == -1))
        {
            std::cout << fmt::format("    /* TunnelHeights */ kNoTunnels,\n");
        }
        else
        {
            std::cout << "    /* TunnelHeights */ std::array<int16_t, 4>{\n";
            std::cout << "        ";
            for (auto th : tunnelHeights)
            {
                if (th == -1)
                {
                    std::cout << "kNoTunnel, ";
                }
                else
                {
                    std::cout << fmt::format("{}, ", th);
                }
            }
            std::cout << "\n";
            std::cout << "    },\n";
        }

        // std::cout << "    /* Segments */ std::array<SegmentFlags, 4>{\n";
        // for (auto& s : segments)
        //{
        //     std::cout << fmt::format("        {},\n", getSegmentFlagsString(s));
        // }
        // std::cout << "    },\n";

        std::cout << fmt::format("    /* Segments */ {},\n", getSegmentFlagsString(enumValue(segments[0])));

        if (segments[1] != rotlSegmentFlags(segments[0], 1))
        {
            // assert(false);
        }
        assert(segments[2] == rotlSegmentFlags(segments[0], 2));
        if (segments[3] != rotlSegmentFlags(segments[0], 3))
        {
            // assert(false);
        }
    }

    std::string printTrack3Alt(TestPaint::Track3& t, uint8_t trackId, uint8_t index)
    {
        auto [ppName, printArray] = printCommonPreamble(t.callOffset, trackId, index);
        if (!printArray)
        {
            return ppName;
        }
        std::cout << "    std::array<std::array<uint32_t, 3>, 4>{\n";
        uint8_t r = 0;
        for (auto& imageId : t.imageIds)
        {
            auto track1 = getDefaultTrackImageName(trackId, index, 1, r);
            auto track2 = getDefaultTrackImageName(trackId, index, 2, r);
            auto track3 = getDefaultTrackImageName(trackId, index, 3, r);
            if (_usedImages.count(imageId[0]))
            {
                track1 = _usedImages[imageId[0]];
            }
            else
            {
                _usedImages.insert(std::make_pair(imageId[0], track1));
            }
            if (_usedImages.count(imageId[1]))
            {
                track2 = _usedImages[imageId[1]];
            }
            else
            {
                _usedImages.insert(std::make_pair(imageId[1], track2));
            }
            if (_usedImages.count(imageId[2]))
            {
                track3 = _usedImages[imageId[2]];
            }
            else
            {
                _usedImages.insert(std::make_pair(imageId[2], track3));
            }
            std::cout << fmt::format("        std::array<uint32_t, 3>{{TrackObj::ImageIds::Style0::{}, TrackObj::ImageIds::Style0::{}, TrackObj::ImageIds::Style0::{}}},\n", track1, track2, track3);
            r++;
        }
        std::cout << "    },\n";

        printCommonAlt(t.boundingBoxOffsets, t.boundingBoxSizes, t.bridgeEdges, t.bridgeQuarters, t.bridgeType, t.tunnelHeight[0], t.segments);

        std::cout << "};\n\n";
        return ppName;
    }

    std::string printTrack1Alt(TestPaint::Track1& t, uint8_t trackId, uint8_t index)
    {
        auto [ppName, printArray] = printCommonPreamble(t.callOffset, trackId, index);
        if (!printArray)
        {
            return ppName;
        }
        std::cout << "    std::array<uint32_t, 4>{\n";
        uint8_t r = 0;
        for (auto& imageId : t.imageIds)
        {
            auto track1 = getDefaultTrackImageName(trackId, index, 0, r);
            if (_usedImages.count(imageId))
            {
                track1 = _usedImages[imageId];
            }
            else
            {
                _usedImages.insert(std::make_pair(imageId, track1));
            }
            std::cout << fmt::format("        TrackObj::ImageIds::Style0::{},\n", track1);
            r++;
        }
        std::cout << "    },\n";

        printCommonAlt(t.boundingBoxOffsets, t.boundingBoxSizes, t.bridgeEdges, t.bridgeQuarters, t.bridgeType, t.tunnelHeight[0], t.segments);

        std::cout << "};\n\n";
        return ppName;
    }

    std::array<uint8_t, 26> kTrackOrder = {
        enumValue(World::Track::TrackId::straight),
        enumValue(World::Track::TrackId::diagonal),
        enumValue(World::Track::TrackId::rightCurveVerySmall),
        enumValue(World::Track::TrackId::leftCurveVerySmall),
        enumValue(World::Track::TrackId::rightCurveSmall),
        enumValue(World::Track::TrackId::leftCurveSmall),
        enumValue(World::Track::TrackId::rightCurve),
        enumValue(World::Track::TrackId::leftCurve),
        enumValue(World::Track::TrackId::rightCurveLarge),
        enumValue(World::Track::TrackId::leftCurveLarge),
        enumValue(World::Track::TrackId::diagonalRightCurveLarge),
        enumValue(World::Track::TrackId::diagonalLeftCurveLarge),
        enumValue(World::Track::TrackId::sBendLeft),
        enumValue(World::Track::TrackId::sBendRight),
        enumValue(World::Track::TrackId::straightSlopeUp),
        enumValue(World::Track::TrackId::straightSlopeDown),
        enumValue(World::Track::TrackId::straightSteepSlopeUp),
        enumValue(World::Track::TrackId::straightSteepSlopeDown),
        enumValue(World::Track::TrackId::rightCurveSmallSlopeUp),
        enumValue(World::Track::TrackId::rightCurveSmallSlopeDown),
        enumValue(World::Track::TrackId::leftCurveSmallSlopeUp),
        enumValue(World::Track::TrackId::leftCurveSmallSlopeDown),
        enumValue(World::Track::TrackId::rightCurveSmallSteepSlopeUp),
        enumValue(World::Track::TrackId::rightCurveSmallSteepSlopeDown),
        enumValue(World::Track::TrackId::leftCurveSmallSteepSlopeUp),
        enumValue(World::Track::TrackId::leftCurveSmallSteepSlopeDown),
    };

    static void printTPT(TestPaint& tp)
    {
        std::vector<std::string> tppNames;
        for (auto trackId : kTrackOrder)
        {
            std::vector<std::string> ppNames;
            auto& track = tp.tracks[trackId];
            for (auto index = 0U; index < track.size(); ++index)
            {
                auto& ti = track[index];
                if (std::holds_alternative<TestPaint::Track3>(ti))
                {
                    auto& t3Ct = std::get<TestPaint::Track3>(ti);
                    ppNames.push_back(printTrack3Alt(t3Ct, trackId, index));
                }
                else if (std::holds_alternative<TestPaint::Track1>(ti))
                {
                    auto& t3Ct = std::get<TestPaint::Track1>(ti);
                    ppNames.push_back(printTrack1Alt(t3Ct, trackId, index));
                }
            }
            if (!track.empty())
            {
                std::string tppName = fmt::format("k{}TPP", _trackIdNames[trackId]);
                tppNames.push_back(tppName);
                std::cout << fmt::format("constexpr std::array<TrackPaintPiece, {}> {} = {{\n", track.size(), tppName);
                for (auto& ppName : ppNames)
                {
                    std::cout << fmt::format("    {},\n", ppName);
                }
                std::cout << "};\n\n";
            }
        }
        std::cout << fmt::format("constexpr std::array<std::span<const TrackPaintPiece>, {}> kTrackPaintParts = {{\n", tppNames.size());
        for (auto trackId = 0; trackId < 26; ++trackId)
        {
            auto tppOffset = std::distance(std::begin(kTrackOrder), std::find(std::begin(kTrackOrder), std::end(kTrackOrder), trackId));

            std::cout << fmt::format("    {},\n", tppNames[tppOffset]);
        }
        assert(tppNames.size() == 26);
        std::cout << "};\n\n";
        for (const auto image : _usedImages)
        {
            std::cout << fmt::format("constexpr uint32_t {} = {};\n", image.second, image.first);
        }
    }

    std::pair<std::string, bool> printCommonPreambleA(const std::array<uint32_t, 4>& callOffsets, uint8_t trackId, uint8_t index, uint8_t paintStyle, int8_t callType)
    {
        uint32_t seenCount = 0;
        uint8_t r = 0;
        for (auto& co : callOffsets)
        {
            if (_outputedOffsetsA[paintStyle].count(co) != 0)
            {
                seenCount++;
            }
            else
            {
                _outputedOffsetsA[paintStyle].insert(std::make_pair(co, OffsetFunc{ trackId, index, r }));
            }
            r++;
        }
        std::string ppName = fmt::format("k{}Addition{}{}", _trackIdNames[trackId], paintStyle, index);

        if (callType == -1)
        {
            fmt::println("constexpr TrackPaintAdditionPiece{} {} = kNullTrackPaintAdditionPiece{};\n", paintStyle, ppName, paintStyle);
            return std::make_pair(ppName, false);
        }
        if (seenCount == 2)
        {
            std::cout << "// Already seen " << seenCount << "\n";
        }
        else if (seenCount == 4)
        {
            uint8_t mirrorTrackId = 0;
            uint8_t mirrorIndex = 0;
            std::array<uint8_t, 4> rotationTable = {};
            for (auto i = 0; i < 4; ++i)
            {
                auto& f = _outputedOffsetsA[paintStyle][callOffsets[i]];
                if (i != 0)
                {
                    assert(mirrorIndex == f.sequence);
                    assert(mirrorTrackId == f.trackId);
                }
                rotationTable[i] = f.rotation;
                mirrorIndex = f.sequence;
                mirrorTrackId = f.trackId;
            }

            std::string mirrorPP = fmt::format("k{}Addition{}{}", _trackIdNames[mirrorTrackId], paintStyle, mirrorIndex);
            std::string rotationTableName = "";
            if (rotationTable[0] == 2 && rotationTable[1] == 3 && rotationTable[2] == 0 && rotationTable[3] == 1)
            {
                rotationTableName = rotationTablesNames[0];
            }
            else if (rotationTable[0] == 3 && rotationTable[1] == 0 && rotationTable[2] == 1 && rotationTable[3] == 2)
            {
                rotationTableName = rotationTablesNames[1];
            }
            else if (rotationTable[0] == 1 && rotationTable[1] == 2 && rotationTable[2] == 3 && rotationTable[3] == 0)
            {
                rotationTableName = rotationTablesNames[2];
            }
            else
            {
                assert(false);
            }
            // assert(rotationTable[0] == 2);
            // assert(rotationTable[1] == 3);
            // assert(rotationTable[2] == 0);
            // assert(rotationTable[3] == 1);

            std::cout << fmt::format("constexpr TrackPaintAdditionPiece{} {} = rotateTrackPP({}, {});\n\n", paintStyle, ppName, mirrorPP, rotationTableName);
            return std::make_pair(ppName, false);
        }
        std::cout << fmt::format("// 0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}\n", callOffsets[0], callOffsets[1], callOffsets[2], callOffsets[3]);
        std::cout << fmt::format("constexpr TrackPaintAdditionPiece{} {} = {{\n", paintStyle, ppName);
        return std::make_pair(ppName, true);
    }
    void printCommonA(const std::array<uint32_t, 4>& images, const std::array<World::Pos3, 4>& boundingBoxOffsets, const std::array<World::Pos3, 4>& boundingBoxSizes, uint8_t paintStyle, uint8_t trackId, uint8_t index)
    {
        std::cout << "    /* ImageIds */ std::array<uint32_t, 4>{\n";
        uint8_t r = 0;
        for (auto& imageId : images)
        {
            auto track1 = getDefaultTrackAdditionImageName(trackId, index, r);
            if (_usedImagesA[paintStyle].count(imageId))
            {
                track1 = _usedImagesA[paintStyle][imageId];
            }
            else
            {
                _usedImagesA[paintStyle].insert(std::make_pair(imageId, track1));
            }
            std::cout << fmt::format("        TrackExtraObj::ImageIds::Style{}::{},\n", paintStyle, track1);
            r++;
        }
        std::cout << "    },\n";

        std::cout << "    /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{\n";
        for (auto& bbO : boundingBoxOffsets)
        {
            std::cout << fmt::format("        World::Pos3{{ {}, {}, {} }},\n", bbO.x, bbO.y, bbO.z);
        }
        std::cout << "    },\n";

        std::cout << "    /* BoundingBoxSizes */ std::array<World::Pos3, 4>{\n";
        for (auto& bbS : boundingBoxSizes)
        {
            std::cout << fmt::format("        World::Pos3{{ {}, {}, {} }},\n", bbS.x, bbS.y, bbS.z);
        }
        std::cout << "    },\n";
    }
    static std::string printTrackAddition0(const TestPaint::TrackAddition& tai, uint8_t trackId, uint8_t index)
    {
        auto [ppName, printArray] = printCommonPreambleA(tai.callOffset, trackId, index, 0, tai.callType);
        if (!printArray)
        {
            return ppName;
        }

        printCommonA(tai.imageIds, tai.boundingBoxOffsets, tai.boundingBoxSizes, 0, trackId, index);

        std::cout << "};\n\n";
        return ppName;
    }
    static std::string printTrackAddition1(const TestPaint::TrackAddition& tai, uint8_t trackId, uint8_t index)
    {
        auto [ppName, printArray] = printCommonPreambleA(tai.callOffset, trackId, index, 1, tai.callType);
        if (!printArray)
        {
            return ppName;
        }

        printCommonA(tai.imageIds, tai.boundingBoxOffsets, tai.boundingBoxSizes, 1, trackId, index);
        if (tai.hasSupports)
        {
            std::cout << "    /* Supports */ TrackAdditionSupport {\n";
            std::cout << "        /* ImageIds */ std::array<uint32_t, 4>{\n";
            uint8_t r = 0;
            for (auto& imageId : tai.supportImageId)
            {
                auto track1 = getDefaultTrackAdditionSupportImageName(trackId, index, r);
                auto track2 = getDefaultTrackAdditionSupportConnectorImageName(trackId, index, r);
                if (_usedImagesA[1].count(imageId))
                {
                    track1 = _usedImagesA[1][imageId];
                    track2 = _usedImagesA[1][imageId + 1];
                }
                else
                {
                    _usedImagesA[1].insert(std::make_pair(imageId, track1));
                    _usedImagesA[1].insert(std::make_pair(imageId + 1, track2));
                }
                std::cout << fmt::format("            {{ TrackExtraObj::ImageIds::Style{}::{}, TrackExtraObj::ImageIds::Style{}::{} }}\n", 1, track1, 1, track2);
                r++;
            }
            std::cout << "        },\n";

            assert((tai.supportHeight[0] == tai.supportHeight[1]) && (tai.supportHeight[0] == tai.supportHeight[2]) && (tai.supportHeight[0] == tai.supportHeight[3]));
            fmt::println("        /* SupportHeight */ {},", tai.supportHeight[0]);
            assert(tai.supportFrequency[0] == tai.supportFrequency[2]);
            assert(tai.supportFrequency[1] == tai.supportFrequency[3]);
            fmt::println("        /* Frequency */ {},", tai.supportFrequency[0]);
            assert(rotlSegmentFlags(SegmentFlags(tai.supportSegment[0]), 1) == SegmentFlags(tai.supportSegment[1]));
            assert(rotlSegmentFlags(SegmentFlags(tai.supportSegment[0]), 2) == SegmentFlags(tai.supportSegment[2]));
            assert(rotlSegmentFlags(SegmentFlags(tai.supportSegment[0]), 3) == SegmentFlags(tai.supportSegment[3]));
            fmt::println("        /* Segment */ {},", getSegmentFlagsString(tai.supportSegment[0]));
            std::cout << "    },\n";
        }
        else
        {
            fmt::println("    /* Supports */ kNoSupports,");
        }
        std::cout << "};\n\n";
        return ppName;
    }

    static void printTPA(TestPaint& tp)
    {
        std::array<std::vector<std::string>, 2> tppNames;
        for (auto i = 0U; i < 2; ++i)
        {
            for (auto trackId : kTrackOrder)
            {
                std::vector<std::string> ppNames;
                auto& trackAdd = tp.trackAdditions[i][trackId];
                for (auto index = 0U; index < trackAdd.size(); ++index)
                {
                    auto& tai = trackAdd[index];
                    if (i == 0)
                    {
                        ppNames.push_back(printTrackAddition0(tai, trackId, index));
                    }
                    else
                    {
                        ppNames.push_back(printTrackAddition1(tai, trackId, index));
                    }
                }
                if (!trackAdd.empty())
                {
                    std::string tppName = fmt::format("k{}TPPA{}", _trackIdNames[trackId], i);
                    tppNames[i].push_back(tppName);
                    std::cout << fmt::format("constexpr std::array<TrackPaintAdditionPiece{}, {}> {} = {{\n", i, trackAdd.size(), tppName);
                    for (auto& ppName : ppNames)
                    {
                        std::cout << fmt::format("    {},\n", ppName);
                    }
                    std::cout << "};\n\n";
                }
            }
        }
        for (auto i = 0U; i < 2; ++i)
        {
            std::cout << fmt::format("constexpr std::array<std::span<const TrackPaintPieceAdditionPiece{}>, {}> kTrackPaintParts{} = {{\n", i, tppNames.size(), i);
            for (auto trackId = 0; trackId < 26; ++trackId)
            {
                auto tppOffset = std::distance(std::begin(kTrackOrder), std::find(std::begin(kTrackOrder), std::end(kTrackOrder), trackId));

                std::cout << fmt::format("    {},\n", tppNames[i][tppOffset]);
            }
            assert(tppNames[i].size() == 26);
            std::cout << "};\n\n";
        }
        for (auto i = 0U; i < 2; ++i)
        {
            for (const auto image : _usedImagesA[i])
            {
                std::cout << fmt::format("constexpr uint32_t {} = {};\n", image.second, image.first);
            }
        }
    }

    void PaintSession::printTP()
    {
        printTPT(tp);
        printTPA(tp);
    }
    static constexpr World::Pos3 rotateBoundBoxSize(const World::Pos3& bbSize, const uint8_t rotation)
    {
        auto output = bbSize;
        // This probably rotates the variables so they're relative to rotation 0.
        // Uses same rotations as directionFlipXAxis
        switch (rotation)
        {
            case 0:
                output.x--;
                output.y--;
                output = World::Pos3{ Math::Vector::rotate(output, 0), output.z };
                break;
            case 1:
                output.x--;
                output = World::Pos3{ Math::Vector::rotate(output, 3), output.z };
                break;
            case 2:
                output = World::Pos3{ Math::Vector::rotate(output, 2), output.z };
                break;
            case 3:
                output.y--;
                output = World::Pos3{ Math::Vector::rotate(output, 1), output.z };
                break;
        }
        return output;
    }

    // 0x004FD140
    PaintStruct* PaintSession::addToPlotListAsParent(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        _lastPS = nullptr;

        auto* ps = createNormalPaintStruct(imageId, offset, boundBoxOffset, boundBoxSize);
        if (ps != nullptr)
        {
            _lastPS = ps;
            addPSToQuadrant(*ps);
        }
        return ps;
    }

    void AddToTP(TestPaint& tp, ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        auto& ct = tp.currentTileTrack;
        if (!ct.isTrack)
        {
            return;
        }
        auto& targetTrack = ct.track;
        if (targetTrack.index() == 0)
        {
            targetTrack = TestPaint::Track1{};
        }
        auto& t1 = std::get<TestPaint::Track1>(targetTrack);
        t1.boundingBoxOffsets[ct.rotation] = boundBoxOffset - World::Pos3{ 0, 0, ct.height };
        t1.boundingBoxSizes[ct.rotation] = boundBoxSize;
        t1.offsets[ct.rotation] = offset - World::Pos3{ 0, 0, ct.height };
        assert(t1.offsets[ct.rotation].x == 0);
        assert(t1.offsets[ct.rotation].y == 0);
        assert(t1.offsets[ct.rotation].z == 0);
        t1.imageIds[ct.rotation] = imageId.getIndex() - ct.baseImageId;
        ct.callCount++;
    }

    void AddToTPA3(TestPaint& tp, ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        auto& ct = tp.currentTileTrackAddition;
        if (!ct.isTrackAddition)
        {
            return;
        }
        auto& t3 = ct.ta;
        t3.boundingBoxOffsets[ct.rotation] = boundBoxOffset - World::Pos3{ 0, 0, ct.height };
        t3.boundingBoxSizes[ct.rotation] = boundBoxSize;
        t3.offsets[ct.rotation] = offset - World::Pos3{ 0, 0, ct.height };
        assert(t3.offsets[ct.rotation].x == 0);
        assert(t3.offsets[ct.rotation].y == 0);
        assert(t3.offsets[ct.rotation].z == 0);
        t3.imageIds[ct.rotation] = imageId.getIndex() - ct.baseImageId;
        t3.callType = 2;
        ct.callCount++;
    }

    // 0x004FD150
    PaintStruct* PaintSession::addToPlotList4FD150(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        // This is identical to addToPlotListAsParent but the offset.x and offset.y are 0
        AddToTP(tp, imageId, offset, boundBoxOffset, boundBoxSize);
        AddToTPA3(tp, imageId, offset, boundBoxOffset, boundBoxSize);
        return addToPlotListAsParent(imageId, offset, boundBoxOffset, boundBoxSize);
    }

    // 0x004FD200
    void PaintSession::addToPlotList4FD200(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId.toUInt32();
        regs.al = offset.x;
        regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        setBoundingBoxOffset(boundBoxOffset);

        // Similar to addToPlotListAsParent but shrinks the bound box based on the rt
        call(_4FD200[currentRotation], regs);
    }

    // 0x004FD1E0
    PaintStruct* PaintSession::addToPlotListAsChild(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        if (*_lastPS == nullptr)
        {
            return addToPlotListAsParent(imageId, offset, boundBoxOffset, boundBoxSize);
        }
        auto* ps = createNormalPaintStruct(imageId, offset, boundBoxOffset, boundBoxSize);
        if (ps == nullptr)
        {
            return nullptr;
        }
        (*_lastPS)->children = ps;
        _lastPS = ps;
        return ps;
    }

    void AddToTP(TestPaint& tp, ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        auto& ct = tp.currentTileTrack;
        if (!ct.isTrack)
        {
            return;
        }
        auto& targetTrack = ct.track;
        if (targetTrack.index() == 0)
        {
            targetTrack = TestPaint::Track3{};
        }
        auto& t3 = std::get<TestPaint::Track3>(targetTrack);
        t3.boundingBoxOffsets[ct.rotation] = boundBoxOffset - World::Pos3{ 0, 0, ct.height };
        t3.boundingBoxSizes[ct.rotation] = boundBoxSize;
        t3.offsets[ct.rotation] = offset - World::Pos3{ 0, 0, ct.height };
        assert(t3.offsets[ct.rotation].x == 0);
        assert(t3.offsets[ct.rotation].y == 0);
        assert(t3.offsets[ct.rotation].z == 0);
        t3.priority[ct.callCount] = priority;
        t3.imageIds[ct.rotation][ct.callCount] = imageId.getIndex() - ct.baseImageId;
        ct.callCount++;
    }

    void AddToTPA(TestPaint& tp, ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        auto& ct = tp.currentTileTrackAddition;
        if (!ct.isTrackAddition)
        {
            return;
        }
        auto& t3 = ct.ta;
        t3.boundingBoxOffsets[ct.rotation] = boundBoxOffset - World::Pos3{ 0, 0, ct.height };
        t3.boundingBoxSizes[ct.rotation] = boundBoxSize;
        t3.offsets[ct.rotation] = offset - World::Pos3{ 0, 0, ct.height };
        assert(t3.offsets[ct.rotation].x == 0);
        assert(t3.offsets[ct.rotation].y == 0);
        assert(t3.offsets[ct.rotation].z == 0);
        t3.priority = priority;
        t3.imageIds[ct.rotation] = imageId.getIndex() - ct.baseImageId;
        t3.callType = 0;
        ct.callCount++;
    }

    TestPaint PaintSession::tp;

    // 0x004FD170
    PaintStruct* PaintSession::addToPlotListTrackRoad(ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        _lastPS = nullptr;
        AddToTP(tp, imageId, priority, offset, boundBoxOffset, boundBoxSize);
        AddToTPA(tp, imageId, priority, offset, boundBoxOffset, boundBoxSize);

        auto* ps = createNormalPaintStruct(imageId, offset, boundBoxOffset, boundBoxSize);
        if (ps != nullptr)
        {
            _lastPS = ps;
            auto* lastTRS = _trackRoadPaintStructs[priority];
            _trackRoadPaintStructs[priority] = ps;
            ps->children = lastTRS;
        }

        return ps;
    }

    void AddToTPATRA(TestPaint& tp, ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        auto& ct = tp.currentTileTrackAddition;
        if (!ct.isTrackAddition)
        {
            return;
        }
        auto& t3 = ct.ta;
        t3.boundingBoxOffsets[ct.rotation] = boundBoxOffset - World::Pos3{ 0, 0, ct.height };
        t3.boundingBoxSizes[ct.rotation] = boundBoxSize;
        t3.offsets[ct.rotation] = offset - World::Pos3{ 0, 0, ct.height };
        assert(t3.offsets[ct.rotation].x == 0);
        assert(t3.offsets[ct.rotation].y == 0);
        assert(t3.offsets[ct.rotation].z == 0);
        t3.priority = priority;
        t3.imageIds[ct.rotation] = imageId.getIndex() - ct.baseImageId;
        t3.callType = 1;
        ct.callCount++;
    }

    // 0x004FD180
    PaintStruct* PaintSession::addToPlotListTrackRoadAddition(ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        _lastPS = nullptr;
        AddToTPATRA(tp, imageId, priority, offset, boundBoxOffset, boundBoxSize);
        auto* ps = createNormalPaintStruct(imageId, offset, boundBoxOffset, boundBoxSize);
        if (ps != nullptr)
        {
            _lastPS = ps;
            auto* lastTRS = _trackRoadAdditionsPaintStructs[priority];
            _trackRoadAdditionsPaintStructs[priority] = ps;
            ps->children = lastTRS;
        }

        return ps;
    }

    // 0x0045E779
    AttachedPaintStruct* PaintSession::attachToPrevious(ImageId imageId, const Ui::Point& offset)
    {
        if (_lastPS == nullptr)
        {
            return nullptr;
        }
        auto* attached = allocatePaintStruct<AttachedPaintStruct>();
        if (attached == nullptr)
        {
            return nullptr;
        }
        attached->imageId = imageId;
        attached->vpPos = offset;
        attached->next = (*_lastPS)->attachedPS;
        (*_lastPS)->attachedPS = attached;
        return attached;
    }

    void PaintSession::init(Gfx::RenderTarget& rt, const SessionOptions& options)
    {
        _renderTarget = &rt;
        _nextFreePaintStruct = &_paintEntries[0];
        _endOfPaintStructArray = &_paintEntries[3998];
        _lastPS = nullptr;
        for (auto& quadrant : _quadrants)
        {
            quadrant = nullptr;
        }
        _quadrantBackIndex = std::numeric_limits<uint32_t>::max();
        _quadrantFrontIndex = 0;
        _lastPaintString = 0;
        _paintStringHead = 0;

        _viewFlags = options.viewFlags;
        currentRotation = options.rotation;

        // TODO: unused
        _foregroundCullingHeight = options.foregroundCullHeight;
    }

    // 0x0045A6CA
    PaintSession* allocateSession(Gfx::RenderTarget& rt, const SessionOptions& options)
    {
        _session.init(rt, options);
        return &_session;
    }

    static PaintStruct* addToPlotListTrackRoadHookHelper(registers& regs, uint8_t rotation)
    {
        PaintSession session;
        const auto imageId = ImageId::fromUInt32(regs.ebx);
        const auto priority = regs.ecx;
        const auto offset = World::Pos3(0, 0, regs.dx);
        const auto boundingBoxSize = World::Pos3(regs.di, regs.si, regs.ah);
        const auto& boundingBoxOffset = session.getBoundingBoxOffset();

        session.setRotation(rotation);
        return session.addToPlotListTrackRoad(imageId, priority, offset, boundingBoxOffset, boundingBoxSize);
    }

    static PaintStruct* addToPlotListTrackRoadAdditionHookHelper(registers& regs, uint8_t rotation)
    {
        PaintSession session;
        const auto imageId = ImageId::fromUInt32(regs.ebx);
        const auto priority = regs.ecx;
        const auto offset = World::Pos3(0, 0, regs.dx);
        const auto boundingBoxSize = World::Pos3(regs.di, regs.si, regs.ah);
        const auto& boundingBoxOffset = session.getBoundingBoxOffset();

        session.setRotation(rotation);
        return session.addToPlotListTrackRoadAddition(imageId, priority, offset, boundingBoxOffset, boundingBoxSize);
    }

    static PaintStruct* addToPlot4FD150HookHelper(registers& regs, uint8_t rotation)
    {
        PaintSession session;
        const auto imageId = ImageId::fromUInt32(regs.ebx);
        const auto offset = World::Pos3(0, 0, regs.dx);
        const auto boundingBoxSize = World::Pos3(regs.di, regs.si, regs.ah);
        const auto& boundingBoxOffset = session.getBoundingBoxOffset();

        session.setRotation(rotation);
        return session.addToPlotList4FD150(imageId, offset, boundingBoxOffset, boundingBoxSize);
    }

    void registerHooks()
    {
        registerHook(
            0x004622A2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                PaintSession session;
                session.generate();

                regs = backup;
                return 0;
            });

        registerHook(
            0x0045BF9F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadHookHelper(regs, 0);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045C0F2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadHookHelper(regs, 1);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045C24C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadHookHelper(regs, 2);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045C3A7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadHookHelper(regs, 3);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });

        registerHook(
            0x0045C503,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadAdditionHookHelper(regs, 0);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045C656,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadAdditionHookHelper(regs, 1);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045C7B0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadAdditionHookHelper(regs, 2);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045C90B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlotListTrackRoadAdditionHookHelper(regs, 3);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });

        registerHook(
            0x0045B405,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlot4FD150HookHelper(regs, 0);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045B58D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlot4FD150HookHelper(regs, 1);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045B721,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlot4FD150HookHelper(regs, 2);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerHook(
            0x0045B8B9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto* ps = addToPlot4FD150HookHelper(regs, 3);
                auto res = ps != nullptr ? X86_FLAG_CARRY : 0;
                regs = backup;
                regs.ebp = X86Pointer(ps);
                return res;
            });
        registerTrackHooks();
    }

    void PaintSession::setSegmentSupportHeight(const SegmentFlags segments, const uint16_t height, const uint8_t slope)
    {
        for (int32_t s = 0; s < 9; s++)
        {
            if ((segments & kSegmentOffsets[s]) != SegmentFlags::none)
            {
                _supportSegments[s].height = height;
                if (height != 0xFFFF)
                {
                    _supportSegments[s].slope = slope;
                    _supportSegments[s].var_03 = 0;
                }
            }
        }
    }

    void PaintSession::setGeneralSupportHeight(const uint16_t height, const uint8_t slope)
    {
        _support->height = height;
        _support->slope = slope;
        _support->var_03 = 0;
    }

    void PaintSession::resetTunnels()
    {
        std::fill(std::begin(_tunnelCounts), std::end(_tunnelCounts), 0);
        _tunnels0[0].height = 0xFF;
        _tunnels1[0].height = 0xFF;
        _tunnels2[0].height = 0xFF;
        _tunnels3[0].height = 0xFF;
    }

    void PaintSession::insertTunnel(coord_t z, uint8_t tunnelType, uint8_t edge)
    {
        TunnelEntry entry{ static_cast<World::MicroZ>(z / World::kMicroZStep), tunnelType };
        auto tunnelCount = _tunnelCounts[edge];
        auto tunnels = getTunnels(edge);
        bool insert = true;
        if (tunnelCount > 0)
        {
            if (tunnels[tunnelCount - 1] == entry)
            {
                insert = false;
            }
        }
        if (insert)
        {
            tunnels[tunnelCount] = entry;
            tunnels[tunnelCount + 1] = { 0xFF, 0xFFU };
            tunnelCount++;
        }
    }

    void PaintSession::insertTunnels(const std::array<int16_t, 4>& tunnelHeights, coord_t height, uint8_t tunnelType)
    {
        for (auto edge = 0U; edge < tunnelHeights.size(); edge++)
        {
            if (tunnelHeights[edge] != -1)
            {
                insertTunnel(tunnelHeights[edge] + height, tunnelType, edge);
            }
        }
    }

    struct GenerationParameters
    {
        World::Pos2 mapLoc;
        uint16_t numVerticalQuadrants;
        std::array<World::Pos2, 5> additionalQuadrants;
        World::Pos2 nextVerticalQuadrant;
    };

    template<uint8_t rotation>
    GenerationParameters generateParameters(Gfx::RenderTarget* rt)
    {
        // TODO: Work out what these constants represent
        uint16_t numVerticalQuadrants = (rt->height + (rotation == 0 ? 1040 : 1056)) >> 5;

        auto mapLoc = Ui::viewportCoordToMapCoord(static_cast<int16_t>(rt->x & 0xFFE0), static_cast<int16_t>((rt->y - 16) & 0xFFE0), 0, rotation);
        if constexpr (rotation & 1)
        {
            mapLoc.y -= 16;
        }
        mapLoc.x &= 0xFFE0;
        mapLoc.y &= 0xFFE0;

        constexpr auto direction = directionFlipXAxis(rotation);
        constexpr std::array<World::Pos2, 5> additionalQuadrants = {
            Math::Vector::rotate(World::Pos2{ -32, 32 }, direction),
            Math::Vector::rotate(World::Pos2{ 0, 32 }, direction),
            Math::Vector::rotate(World::Pos2{ 32, 0 }, direction),
            Math::Vector::rotate(World::Pos2{ 32, -32 }, direction),
            Math::Vector::rotate(World::Pos2{ -32, 64 }, direction),
        };
        constexpr auto nextVerticalQuadrant = Math::Vector::rotate(World::Pos2{ 32, 32 }, direction);

        return { mapLoc, numVerticalQuadrants, additionalQuadrants, nextVerticalQuadrant };
    }

    void PaintSession::generateTilesAndEntities(GenerationParameters&& p)
    {
        for (; p.numVerticalQuadrants > 0; --p.numVerticalQuadrants)
        {
            paintTileElements(*this, p.mapLoc);
            paintEntities(*this, p.mapLoc);

            auto loc1 = p.mapLoc + p.additionalQuadrants[0];
            paintTileElements2(*this, loc1);
            paintEntities(*this, loc1);

            auto loc2 = p.mapLoc + p.additionalQuadrants[1];
            paintTileElements(*this, loc2);
            paintEntities(*this, loc2);

            auto loc3 = p.mapLoc + p.additionalQuadrants[2];
            paintTileElements2(*this, loc3);
            paintEntities(*this, loc3);

            auto loc4 = p.mapLoc + p.additionalQuadrants[3];
            paintEntities2(*this, loc4);

            auto loc5 = p.mapLoc + p.additionalQuadrants[4];
            paintEntities2(*this, loc5);

            p.mapLoc += p.nextVerticalQuadrant;
        }
    }

    void PaintSession::attachStringStruct(PaintStringStruct& psString)
    {
        auto* previous = *_lastPaintString;
        _lastPaintString = &psString;
        if (previous == nullptr)
        {
            _paintStringHead = &psString;
        }
        else
        {
            previous->next = &psString;
        }
    }

    PaintStruct* PaintSession::createNormalPaintStruct(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        auto* const g1 = Gfx::getG1Element(imageId.getIndex());
        if (g1 == nullptr)
        {
            return nullptr;
        }

        const auto swappedRotation = directionFlipXAxis(currentRotation);
        auto swappedRotCoord = World::Pos3{ Math::Vector::rotate(offset, swappedRotation), offset.z };
        swappedRotCoord += World::Pos3{ getSpritePosition(), 0 };

        const auto vpPos = World::gameToScreen(swappedRotCoord, currentRotation);

        if (!imageWithinRT(vpPos, *g1, **_renderTarget))
        {
            return nullptr;
        }

        const auto rotBoundBoxOffset = World::Pos3{ Math::Vector::rotate(boundBoxOffset, swappedRotation), boundBoxOffset.z };
        const auto rotBoundBoxSize = rotateBoundBoxSize(boundBoxSize, currentRotation);

        auto* ps = allocatePaintStruct<PaintStruct>();
        if (ps == nullptr)
        {
            return nullptr;
        }

        ps->imageId = imageId;
        ps->vpPos.x = vpPos.x;
        ps->vpPos.y = vpPos.y;
        ps->bounds.xEnd = rotBoundBoxSize.x + rotBoundBoxOffset.x + getSpritePosition().x;
        ps->bounds.yEnd = rotBoundBoxSize.y + rotBoundBoxOffset.y + getSpritePosition().y;
        ps->bounds.zEnd = rotBoundBoxSize.z + rotBoundBoxOffset.z;
        ps->bounds.x = rotBoundBoxOffset.x + getSpritePosition().x;
        ps->bounds.y = rotBoundBoxOffset.y + getSpritePosition().y;
        ps->bounds.z = rotBoundBoxOffset.z;
        ps->flags = PaintStructFlags::none;
        ps->attachedPS = nullptr;
        ps->children = nullptr;
        ps->type = _itemType;
        ps->modId = _trackModId;
        ps->mapPos = _mapPosition;
        ps->tileElement = reinterpret_cast<World::TileElement*>(*_currentItem);
        return ps;
    }

    // 0x004622A2
    void PaintSession::generate()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
            return;

        currentRotation = Ui::WindowManager::getCurrentRotation();
        switch (currentRotation)
        {
            case 0:
                generateTilesAndEntities(generateParameters<0>(getRenderTarget()));
                break;
            case 1:
                generateTilesAndEntities(generateParameters<1>(getRenderTarget()));
                break;
            case 2:
                generateTilesAndEntities(generateParameters<2>(getRenderTarget()));
                break;
            case 3:
                generateTilesAndEntities(generateParameters<3>(getRenderTarget()));
                break;
        }
    }

    template<uint8_t>
    static bool checkBoundingBox(const PaintStructBoundBox& initialBBox, const PaintStructBoundBox& currentBBox);

    template<>
    bool checkBoundingBox<0>(const PaintStructBoundBox& initialBBox, const PaintStructBoundBox& currentBBox)
    {
        if (initialBBox.zEnd >= currentBBox.z && initialBBox.yEnd >= currentBBox.y && initialBBox.xEnd >= currentBBox.x
            && !(initialBBox.z < currentBBox.zEnd && initialBBox.y < currentBBox.yEnd && initialBBox.x < currentBBox.xEnd))
        {
            return true;
        }
        return false;
    }

    template<>
    bool checkBoundingBox<1>(const PaintStructBoundBox& initialBBox, const PaintStructBoundBox& currentBBox)
    {
        if (initialBBox.zEnd >= currentBBox.z && initialBBox.yEnd >= currentBBox.y && initialBBox.xEnd < currentBBox.x
            && !(initialBBox.z < currentBBox.zEnd && initialBBox.y < currentBBox.yEnd && initialBBox.x >= currentBBox.xEnd))
        {
            return true;
        }
        return false;
    }

    template<>
    bool checkBoundingBox<2>(const PaintStructBoundBox& initialBBox, const PaintStructBoundBox& currentBBox)
    {
        if (initialBBox.zEnd >= currentBBox.z && initialBBox.yEnd < currentBBox.y && initialBBox.xEnd < currentBBox.x
            && !(initialBBox.z < currentBBox.zEnd && initialBBox.y >= currentBBox.yEnd && initialBBox.x >= currentBBox.xEnd))
        {
            return true;
        }
        return false;
    }

    template<>
    bool checkBoundingBox<3>(const PaintStructBoundBox& initialBBox, const PaintStructBoundBox& currentBBox)
    {
        if (initialBBox.zEnd >= currentBBox.z && initialBBox.yEnd < currentBBox.y && initialBBox.xEnd >= currentBBox.x
            && !(initialBBox.z < currentBBox.zEnd && initialBBox.y >= currentBBox.yEnd && initialBBox.x < currentBBox.xEnd))
        {
            return true;
        }
        return false;
    }

    template<uint8_t _TRotation>
    static PaintStruct* arrangeStructsHelperRotation(PaintStruct* psNext, const uint16_t quadrantIndex, const QuadrantFlags flag)
    {
        PaintStruct* ps = nullptr;

        // Get the first node in the specified quadrant.
        do
        {
            ps = psNext;
            psNext = psNext->nextQuadrantPS;
            if (psNext == nullptr)
                return ps;
        } while (quadrantIndex > psNext->quadrantIndex);

        // We keep track of the first node in the quadrant so the next call with a higher quadrant index
        // can use this node to skip some iterations.
        auto* psQuadrantEntry = ps;

        // Visit all nodes in the linked quadrant list and determine their current
        // sorting relevancy.
        auto* psTemp = ps;
        do
        {
            ps = ps->nextQuadrantPS;
            if (ps == nullptr)
                break;

            if (ps->quadrantIndex > quadrantIndex + 1)
            {
                // Outside of the range.
                ps->quadrantFlags = QuadrantFlags::outsideQuadrant;
            }
            else if (ps->quadrantIndex == quadrantIndex + 1)
            {
                // Is neighbour and requires a visit.
                ps->quadrantFlags = QuadrantFlags::neighbour | QuadrantFlags::pendingVisit;
            }
            else if (ps->quadrantIndex == quadrantIndex)
            {
                // In specified quadrant, requires visit.
                ps->quadrantFlags = flag | QuadrantFlags::pendingVisit;
            }
        } while (ps->quadrantIndex <= quadrantIndex + 1);
        ps = psTemp;

        // Iterate all nodes in the current list and re-order them based on
        // the current rotation and their bounding box.
        while (true)
        {
            // Get the first pending node in the quadrant list
            while (true)
            {
                psNext = ps->nextQuadrantPS;
                if (psNext == nullptr)
                {
                    // End of the current list.
                    return psQuadrantEntry;
                }
                if (psNext->hasQuadrantFlags(QuadrantFlags::outsideQuadrant))
                {
                    // Reached point outside of specified quadrant.
                    return psQuadrantEntry;
                }
                if (psNext->hasQuadrantFlags(QuadrantFlags::pendingVisit))
                {
                    // Found node to check on.
                    break;
                }
                ps = psNext;
            }

            // Mark visited.
            psNext->quadrantFlags &= ~QuadrantFlags::pendingVisit;
            psTemp = ps;

            // Compare current node against the remaining children.
            const PaintStructBoundBox& initialBBox = psNext->bounds;
            while (true)
            {
                ps = psNext;
                psNext = psNext->nextQuadrantPS;
                if (psNext == nullptr)
                    break;
                if (psNext->hasQuadrantFlags(QuadrantFlags::outsideQuadrant))
                    break;
                if (!psNext->hasQuadrantFlags(QuadrantFlags::neighbour))
                    continue;

                const PaintStructBoundBox& currentBBox = psNext->bounds;

                const bool compareResult = checkBoundingBox<_TRotation>(initialBBox, currentBBox);

                if (compareResult)
                {
                    // Child node intersects with current node, move behind.
                    ps->nextQuadrantPS = psNext->nextQuadrantPS;
                    PaintStruct* ps_temp2 = psTemp->nextQuadrantPS;
                    psTemp->nextQuadrantPS = psNext;
                    psNext->nextQuadrantPS = ps_temp2;
                    psNext = ps;
                }
            }

            ps = psTemp;
        }
    }

    static PaintStruct* arrangeStructsHelper(PaintStruct* psNext, uint16_t quadrantIndex, QuadrantFlags flag, uint8_t rotation)
    {
        switch (rotation)
        {
            case 0:
                return arrangeStructsHelperRotation<0>(psNext, quadrantIndex, flag);
            case 1:
                return arrangeStructsHelperRotation<1>(psNext, quadrantIndex, flag);
            case 2:
                return arrangeStructsHelperRotation<2>(psNext, quadrantIndex, flag);
            case 3:
                return arrangeStructsHelperRotation<3>(psNext, quadrantIndex, flag);
        }
        return nullptr;
    }

    // 0x0045E7B5
    void PaintSession::arrangeStructs()
    {
        _paintHead = _nextFreePaintStruct;
        _nextFreePaintStruct++;

        PaintStruct* ps = &(*_paintHead)->basic;
        ps->nextQuadrantPS = nullptr;

        uint32_t quadrantIndex = _quadrantBackIndex;
        if (quadrantIndex == std::numeric_limits<uint32_t>::max())
        {
            return;
        }

        do
        {
            PaintStruct* psNext = _quadrants[quadrantIndex];
            if (psNext != nullptr)
            {
                ps->nextQuadrantPS = psNext;
                do
                {
                    ps = psNext;
                    psNext = psNext->nextQuadrantPS;

                } while (psNext != nullptr);
            }
        } while (++quadrantIndex <= _quadrantFrontIndex);

        PaintStruct* psCache = arrangeStructsHelper(
            &(*_paintHead)->basic, _quadrantBackIndex & 0xFFFF, QuadrantFlags::neighbour, currentRotation);

        quadrantIndex = _quadrantBackIndex;
        while (++quadrantIndex < _quadrantFrontIndex)
        {
            psCache = arrangeStructsHelper(psCache, quadrantIndex & 0xFFFF, QuadrantFlags::none, currentRotation);
        }
    }

    static bool isTypeCullableBuilding(const Ui::ViewportInteraction::InteractionItem type)
    {
        switch (type)
        {
            case Ui::ViewportInteraction::InteractionItem::building:
            case Ui::ViewportInteraction::InteractionItem::industry:
            case Ui::ViewportInteraction::InteractionItem::headquarterBuilding:
                return true;
            default:
                return false;
        }
    }

    static bool isTypeCullableRoad(const Ui::ViewportInteraction::InteractionItem type)
    {
        switch (type)
        {
            case Ui::ViewportInteraction::InteractionItem::roadStation:
            case Ui::ViewportInteraction::InteractionItem::road:
            case Ui::ViewportInteraction::InteractionItem::roadExtra:
                return true;
            default:
                return false;
        }
    }

    static bool isTypeCullableScenery(const Ui::ViewportInteraction::InteractionItem type)
    {
        switch (type)
        {
            case Ui::ViewportInteraction::InteractionItem::wall:
                return true;
            default:
                return false;
        }
    }

    static bool isTypeCullableTrack(const Ui::ViewportInteraction::InteractionItem type)
    {
        switch (type)
        {
            case Ui::ViewportInteraction::InteractionItem::track:
            case Ui::ViewportInteraction::InteractionItem::trackExtra:
            case Ui::ViewportInteraction::InteractionItem::signal:
            case Ui::ViewportInteraction::InteractionItem::trainStation:
            case Ui::ViewportInteraction::InteractionItem::airport:
            case Ui::ViewportInteraction::InteractionItem::dock:
            case Ui::ViewportInteraction::InteractionItem::bridge:
                return true;
            default:
                return false;
        }
    }

    static bool isTypeCullableTree(const Ui::ViewportInteraction::InteractionItem type)
    {
        switch (type)
        {
            case Ui::ViewportInteraction::InteractionItem::industryTree:
            case Ui::ViewportInteraction::InteractionItem::tree:
                return true;
            default:
                return false;
        }
    }

    static bool shouldTryCullPaintStruct(const PaintStruct& ps, const Ui::ViewportFlags viewFlags)
    {
        if ((viewFlags & Ui::ViewportFlags::seeThroughTracks) != Ui::ViewportFlags::none)
        {
            if (isTypeCullableTrack(ps.type))
            {
                return true;
            }
        }
        if ((viewFlags & Ui::ViewportFlags::seeThroughRoads) != Ui::ViewportFlags::none)
        {
            if (isTypeCullableRoad(ps.type))
            {
                return true;
            }
        }
        if ((viewFlags & Ui::ViewportFlags::seeThroughTrees) != Ui::ViewportFlags::none)
        {
            if (isTypeCullableTree(ps.type))
            {
                return true;
            }
        }
        if ((viewFlags & Ui::ViewportFlags::seeThroughScenery) != Ui::ViewportFlags::none)
        {
            if (isTypeCullableScenery(ps.type))
            {
                return true;
            }
        }
        if ((viewFlags & Ui::ViewportFlags::seeThroughBuildings) != Ui::ViewportFlags::none)
        {
            if (isTypeCullableBuilding(ps.type))
            {
                return true;
            }
        }
        return false;
    }

    static bool cullPaintStructImage(const ImageId& imageId, const Ui::ViewportFlags viewFlags)
    {
        if ((viewFlags & Ui::ViewportFlags::underground_view) != Ui::ViewportFlags::none)
        {
            return true;
        }
        if (imageId.isBlended())
        {
            return true;
        }
        return false;
    }

    static void drawStruct(Gfx::RenderTarget& rt, Drawing::SoftwareDrawingContext& drawingCtx, const PaintStruct& ps, const bool shouldCull)
    {
        auto imageId = ps.imageId;

        if (shouldCull)
        {
            imageId = ImageId(imageId.getIndex()).withTranslucency(ExtColour::unk30);
        }
        Ui::Point imagePos = ps.vpPos;
        if (ps.type == Ui::ViewportInteraction::InteractionItem::entity)
        {
            switch (rt.zoomLevel)
            {
                case 0:
                    break;
                case 1:
                    imagePos.x &= 0xFFFE;
                    imagePos.y &= 0xFFFE;
                    break;
                case 2:
                    imagePos.x &= 0xFFFC;
                    imagePos.y &= 0xFFFC;
                    break;
                case 3:
                    imagePos.x &= 0xFFF8;
                    imagePos.y &= 0xFFF8;
                    break;
            }
        }
        if ((ps.flags & PaintStructFlags::hasMaskedImage) != PaintStructFlags::none)
        {
            drawingCtx.drawImageMasked(rt, imagePos, imageId, ps.maskedImageId);
        }
        else
        {
            drawingCtx.drawImage(rt, imagePos, imageId);
        }
    }

    static void drawAttachStruct(Gfx::RenderTarget& rt, Drawing::SoftwareDrawingContext& drawingCtx, const PaintStruct& ps, const AttachedPaintStruct& attachPs, const bool shouldCull)
    {
        auto imageId = attachPs.imageId;

        if (shouldCull)
        {
            imageId = ImageId(imageId.getIndex()).withTranslucency(ExtColour::unk30);
        }
        Ui::Point imagePos = ps.vpPos + attachPs.vpPos;
        if (rt.zoomLevel != 0)
        {
            imagePos.x &= 0xFFFE;
            imagePos.y &= 0xFFFE;
        }

        if ((attachPs.flags & PaintStructFlags::hasMaskedImage) != PaintStructFlags::none)
        {
            drawingCtx.drawImageMasked(rt, imagePos, imageId, attachPs.maskedImageId);
        }
        else
        {
            drawingCtx.drawImage(rt, imagePos, imageId);
        }
    }

    // 0x0045EA23
    void PaintSession::drawStructs()
    {
        Gfx::RenderTarget& rt = **_renderTarget;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (const auto* ps = (*_paintHead)->basic.nextQuadrantPS; ps != nullptr; ps = ps->nextQuadrantPS)
        {
            const bool shouldCull = shouldTryCullPaintStruct(*ps, _viewFlags);

            if (shouldCull)
            {
                if (cullPaintStructImage(ps->imageId, _viewFlags))
                {
                    continue;
                }
            }

            drawStruct(rt, drawingCtx, *ps, shouldCull);

            // Draw any children this might have
            for (const auto* childPs = ps->children; childPs != nullptr; childPs = childPs->children)
            {
                // assert(childPs->attachedPS == nullptr); Children can have attachments but we are skipping them to be investigated!
                const bool shouldCullChild = shouldTryCullPaintStruct(*childPs, _viewFlags);

                if (shouldCullChild)
                {
                    if (cullPaintStructImage(childPs->imageId, _viewFlags))
                    {
                        continue;
                    }
                }

                drawStruct(rt, drawingCtx, *childPs, shouldCullChild);
            }

            // Draw any attachments to the struct
            for (const auto* attachPs = ps->attachedPS; attachPs != nullptr; attachPs = attachPs->next)
            {
                const bool shouldCullAttach = shouldTryCullPaintStruct(*ps, _viewFlags);
                if (shouldCullAttach)
                {
                    if (cullPaintStructImage(attachPs->imageId, _viewFlags))
                    {
                        continue;
                    }
                }
                drawAttachStruct(rt, drawingCtx, *ps, *attachPs, shouldCullAttach);
            }
        }
    }

    // 0x0045A60E
    void PaintSession::drawStringStructs()
    {
        PaintStringStruct* psString = _paintStringHead;
        if (psString == nullptr)
        {
            return;
        }

        Gfx::RenderTarget unZoomedRt = **(_renderTarget);
        const auto zoom = (*_renderTarget)->zoomLevel;

        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= zoom;
        unZoomedRt.y >>= zoom;
        unZoomedRt.width >>= zoom;
        unZoomedRt.height >>= zoom;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(zoom == 0 ? Font::medium_bold : Font::small);

        char buffer[512]{};

        for (; psString != nullptr; psString = psString->next)
        {
            Ui::Point loc(psString->vpPos.x >> zoom, psString->vpPos.y >> zoom);
            StringManager::formatString(buffer, psString->stringId, psString->args);

            Ui::WindowManager::setWindowColours(0, AdvancedColour(static_cast<Colour>(psString->colour)));
            Ui::WindowManager::setWindowColours(1, AdvancedColour(static_cast<Colour>(psString->colour)));

            drawingCtx.drawStringYOffsets(unZoomedRt, loc, Colour::black, buffer, psString->yOffsets);
        }
    }

    // 0x00447A5F
    static bool isSpriteInteractedWithPaletteSet(Gfx::RenderTarget* rt, uint32_t imageId, const Ui::Point& coords, const Gfx::PaletteMap::View paletteMap)
    {
        static loco_global<const uint8_t*, 0x0050B860> _paletteMap;
        static loco_global<bool, 0x00E40114> _interactionResult;
        _paletteMap = paletteMap.data();
        registers regs{};
        regs.ebx = imageId;
        regs.edi = X86Pointer(rt);
        regs.cx = coords.x;
        regs.dx = coords.y;
        call(0x00447A5F, regs);
        return _interactionResult;
    }

    // 0x00447A0E
    static bool isSpriteInteractedWith(Gfx::RenderTarget* rt, ImageId imageId, const Ui::Point& coords)
    {
        static loco_global<bool, 0x00E40114> _interactionResult;
        static loco_global<uint32_t, 0x00E04324> _interactionFlags;
        _interactionResult = false;
        auto paletteMap = Gfx::PaletteMap::getDefault();
        if (imageId.hasPrimary())
        {
            _interactionFlags = Gfx::ImageIdFlags::remap;
            ExtColour index = imageId.hasSecondary() ? static_cast<ExtColour>(imageId.getPrimary()) : imageId.getRemap();
            if (auto pm = Gfx::PaletteMap::getForColour(index))
            {
                paletteMap = *pm;
            }
        }
        else
        {
            _interactionFlags = 0;
        }
        return isSpriteInteractedWithPaletteSet(rt, imageId.getIndex(), coords, paletteMap);
    }

    // 0x0045EDFC
    static bool isPSSpriteTypeInFilter(const InteractionItem spriteType, InteractionItemFlags filter)
    {
        constexpr InteractionItemFlags interactionItemToFilter[] = {
            InteractionItemFlags::none,
            InteractionItemFlags::surface,
            InteractionItemFlags::surface,
            InteractionItemFlags::entity,
            InteractionItemFlags::track,
            InteractionItemFlags::trackExtra,
            InteractionItemFlags::signal,
            InteractionItemFlags::station,
            InteractionItemFlags::station,
            InteractionItemFlags::station,
            InteractionItemFlags::station,
            InteractionItemFlags::water,
            InteractionItemFlags::tree,
            InteractionItemFlags::wall,
            InteractionItemFlags::townLabel,
            InteractionItemFlags::stationLabel,
            InteractionItemFlags::roadAndTram,
            InteractionItemFlags::roadAndTramExtra,
            InteractionItemFlags::none,
            InteractionItemFlags::building,
            InteractionItemFlags::industry,
            InteractionItemFlags::headquarterBuilding,
        };

        if (spriteType == InteractionItem::noInteraction
            || spriteType == InteractionItem::bridge) // 18 as a type seems to not exist.
            return false;

        InteractionItemFlags mask = interactionItemToFilter[static_cast<size_t>(spriteType)];

        if ((filter & mask) != InteractionItemFlags::none)
        {
            return false;
        }

        return true;
    }

    // 0x0045ED91
    [[nodiscard]] InteractionArg PaintSession::getNormalInteractionInfo(const InteractionItemFlags flags)
    {
        InteractionArg info{};

        for (auto* ps = (*_paintHead)->basic.nextQuadrantPS; ps != nullptr; ps = ps->nextQuadrantPS)
        {
            // Check main paint struct
            if (isSpriteInteractedWith(getRenderTarget(), ps->imageId, ps->vpPos))
            {
                if (isPSSpriteTypeInFilter(ps->type, flags))
                {
                    info = { *ps };
                }
            }

            // Check children paint structs
            for (const auto* childPs = ps->children; childPs != nullptr; childPs = childPs->children)
            {
                if (isSpriteInteractedWith(getRenderTarget(), childPs->imageId, childPs->vpPos))
                {
                    if (isPSSpriteTypeInFilter(childPs->type, flags))
                    {
                        info = { *childPs };
                    }
                }
            }

            // Check attached to main paint struct
            for (auto* attachedPS = ps->attachedPS; attachedPS != nullptr; attachedPS = attachedPS->next)
            {
                if (isSpriteInteractedWith(getRenderTarget(), attachedPS->imageId, attachedPS->vpPos + ps->vpPos))
                {
                    if (isPSSpriteTypeInFilter(ps->type, flags))
                    {
                        info = { *ps };
                    }
                }
            }
        }
        return info;
    }

    // 0x0048DDE4
    [[nodiscard]] InteractionArg PaintSession::getStationNameInteractionInfo(const InteractionItemFlags flags)
    {
        InteractionArg interaction{};

        if ((flags & InteractionItemFlags::stationLabel) != InteractionItemFlags::none)
        {
            return interaction;
        }

        auto rect = (*_renderTarget)->getDrawableRect();

        for (auto& station : StationManager::stations())
        {
            if ((station.flags & StationFlags::flag_5) != StationFlags::none)
            {
                continue;
            }

            if (!station.labelFrame.contains(rect, (*_renderTarget)->zoomLevel))
            {
                continue;
            }

            interaction.type = InteractionItem::stationLabel;
            interaction.value = enumValue(station.id());
            interaction.pos.x = station.x;
            interaction.pos.y = station.y;
        }
        return interaction;
    }

    // 0x0049773D
    [[nodiscard]] InteractionArg PaintSession::getTownNameInteractionInfo(const InteractionItemFlags flags)
    {
        InteractionArg interaction{};

        if ((flags & InteractionItemFlags::townLabel) != InteractionItemFlags::none)
        {
            return interaction;
        }

        auto rect = (*_renderTarget)->getDrawableRect();

        for (auto& town : TownManager::towns())
        {
            if (!town.labelFrame.contains(rect, (*_renderTarget)->zoomLevel))
            {
                continue;
            }

            interaction.type = InteractionItem::townLabel;
            interaction.value = enumValue(town.id());
            interaction.pos.x = town.x;
            interaction.pos.y = town.y;
        }
        return interaction;
    }

    static PaintStruct* getLastChild(PaintStruct& ps)
    {
        auto* lastChild = &ps;
        for (; lastChild->children != nullptr; lastChild = lastChild->children)
            ;
        return lastChild;
    }

    static PaintStruct* appendTogetherStructs(std::span<PaintStruct*> paintStructs)
    {
        PaintStruct* routeEntry = nullptr;
        // Append all of the paint structs together onto the route entry as children in order of array
        PaintStruct* lastEntry = nullptr;
        for (auto* entry : paintStructs)
        {
            if (entry == nullptr)
            {
                continue;
            }
            if (lastEntry == nullptr)
            {
                routeEntry = entry;
            }
            else
            {
                lastEntry->children = entry;
            }
            lastEntry = getLastChild(*entry);
        }

        // Reset table now that we have appended into one
        std::fill(std::begin(paintStructs), std::end(paintStructs), nullptr);
        return routeEntry;
    }

    // 0x0045CABF, 0x0045CAF4, 0x0045CB29, 0x0045CB5A, 0x0045CC73, 0x0045CCA8, 0x0045CCDD, 0x0045CD0E
    static PaintStructBoundBox getMinMaxXYBounding(PaintStruct& routeEntry, uint8_t rotation)
    {
        auto minMaxBounds = routeEntry.bounds;
        switch (rotation)
        {
            case 0:
                for (auto* child = routeEntry.children; child != nullptr; child = child->children)
                {
                    minMaxBounds.x = std::max(minMaxBounds.x, child->bounds.x);
                    minMaxBounds.y = std::max(minMaxBounds.y, child->bounds.y);
                    minMaxBounds.xEnd = std::min(minMaxBounds.xEnd, child->bounds.xEnd);
                    minMaxBounds.yEnd = std::min(minMaxBounds.yEnd, child->bounds.yEnd);
                }
                break;
            case 1:
                for (auto* child = routeEntry.children; child != nullptr; child = child->children)
                {
                    minMaxBounds.x = std::min(minMaxBounds.x, child->bounds.x);
                    minMaxBounds.y = std::max(minMaxBounds.y, child->bounds.y);
                    minMaxBounds.xEnd = std::max(minMaxBounds.xEnd, child->bounds.xEnd);
                    minMaxBounds.yEnd = std::min(minMaxBounds.yEnd, child->bounds.yEnd);
                }
                break;
            case 2:
                for (auto* child = routeEntry.children; child != nullptr; child = child->children)
                {
                    minMaxBounds.x = std::min(minMaxBounds.x, child->bounds.x);
                    minMaxBounds.y = std::min(minMaxBounds.y, child->bounds.y);
                    minMaxBounds.xEnd = std::max(minMaxBounds.xEnd, child->bounds.xEnd);
                    minMaxBounds.yEnd = std::max(minMaxBounds.yEnd, child->bounds.yEnd);
                }
                break;
            case 3:
                for (auto* child = routeEntry.children; child != nullptr; child = child->children)
                {
                    minMaxBounds.x = std::max(minMaxBounds.x, child->bounds.x);
                    minMaxBounds.y = std::min(minMaxBounds.y, child->bounds.y);
                    minMaxBounds.xEnd = std::min(minMaxBounds.xEnd, child->bounds.xEnd);
                    minMaxBounds.yEnd = std::max(minMaxBounds.yEnd, child->bounds.yEnd);
                }
                break;
        }
        return minMaxBounds;
    }

    void PaintSession::finaliseOrdering(std::span<PaintStruct*> paintStructs)
    {
        auto* routeEntry = appendTogetherStructs(paintStructs);

        if (routeEntry == nullptr)
        {
            return;
        }

        auto minMaxBounds = getMinMaxXYBounding(*routeEntry, getRotation());

        routeEntry->bounds.x = minMaxBounds.x;
        routeEntry->bounds.y = minMaxBounds.y;
        routeEntry->bounds.xEnd = minMaxBounds.xEnd;
        routeEntry->bounds.yEnd = minMaxBounds.yEnd;

        addPSToQuadrant(*routeEntry);
    }

    // 0x0045CA67
    void PaintSession::finaliseTrackRoadOrdering()
    {
        finaliseOrdering(std::span<PaintStruct*>(&_trackRoadPaintStructs[0], &_trackRoadPaintStructs[0] + std::size(_trackRoadPaintStructs)));
    }

    // 0x0045CC1B
    void PaintSession::finaliseTrackRoadAdditionsOrdering()
    {
        finaliseOrdering(std::span<PaintStruct*>(&_trackRoadAdditionsPaintStructs[0], &_trackRoadAdditionsPaintStructs[0] + std::size(_trackRoadAdditionsPaintStructs)));
    }

    // Note: Size includes for 1 extra at end that should never be anything other than 0xFF, 0xFF or 0, 0
    std::span<TunnelEntry> PaintSession::getTunnels(uint8_t edge)
    {
        switch (edge)
        {
            case 0:
                return std::span<TunnelEntry>(&_tunnels0[0], _tunnels0.size());
            case 1:
                return std::span<TunnelEntry>(&_tunnels1[0], _tunnels1.size());
            case 2:
                return std::span<TunnelEntry>(&_tunnels2[0], _tunnels2.size());
            case 3:
                return std::span<TunnelEntry>(&_tunnels3[0], _tunnels3.size());
        }
        return std::span<TunnelEntry>();
    }
}
