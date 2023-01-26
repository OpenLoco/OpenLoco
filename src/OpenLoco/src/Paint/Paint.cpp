#include "Paint.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringManager.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "PaintEntity.h"
#include "PaintTile.h"
#include "PaintTrack.h"
#include "Scenario.h"
#include "StationManager.h"
#include "TownManager.h"
#include "Ui.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    PaintSession _session;

    void PaintSession::setEntityPosition(const Map::Pos2& pos)
    {
        _spritePositionX = pos.x;
        _spritePositionY = pos.y;
    }
    void PaintSession::setMapPosition(const Map::Pos2& pos)
    {
        _mapPosition = pos;
    }
    void PaintSession::setUnkPosition(const Map::Pos2& pos)
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
        _525CE4[0] = 0xFFFF;
        _525CF0 = 0;
        _525CF0 = 0;
        _525CF8 = 0;
        _F003F4 = 0;
        _F003F6 = 0;
        std::fill(std::begin(_unkSegments), std::end(_unkSegments), 0);
        std::fill(std::begin(_E400D0), std::end(_E400D0), nullptr);
        std::fill(std::begin(_E400E4), std::end(_E400E4), nullptr);
        _112C300 = 0;
        _112C306 = 0;
    }

    void PaintSession::setMaxHeight(const Map::Pos2& loc)
    {
        auto tile = Map::TileManager::get(loc);
        uint8_t maxClearZ = 0;
        for (const auto& el : tile)
        {
            maxClearZ = std::max(maxClearZ, el.clearZ());
            const auto* surface = el.as<Map::SurfaceElement>();
            if (!surface)
            {
                continue;
            }

            if (surface->water())
            {
                maxClearZ = std::max<uint8_t>(maxClearZ, surface->water() * Map::kMicroToSmallZStep);
            }
            if (surface->isIndustrial())
            {
                maxClearZ = std::max<uint8_t>(maxClearZ, surface->clearZ() + 24);
            }
        }
        _maxHeight = (maxClearZ * Map::kSmallZStep) + 32;
    }

    loco_global<int32_t[4], 0x4FD120> _addToStringPlotList;
    loco_global<int32_t[4], 0x4FD130> _4FD130;
    loco_global<int32_t[4], 0x4FD140> _4FD140;
    loco_global<int32_t[4], 0x4FD150> _4FD150;
    loco_global<int32_t[4], 0x4FD1E0> _4FD1E0;
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
        constexpr auto mapRangeMax = kMaxPaintQuadrants * Map::kTileSize;
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
        const uint32_t paintQuadrantIndex = std::clamp(positionHash / Map::kTileSize, 0, kMaxPaintQuadrants - 1);

        ps.quadrantIndex = paintQuadrantIndex;
        ps.nextQuadrantPS = _quadrants[paintQuadrantIndex];
        _quadrants[paintQuadrantIndex] = &ps;

        _quadrantBackIndex = std::min(*_quadrantBackIndex, paintQuadrantIndex);
        _quadrantFrontIndex = std::max(*_quadrantFrontIndex, paintQuadrantIndex);
    }
    // 0x004FD120
    PaintStringStruct* PaintSession::addToStringPlotList(const uint32_t amount, const string_id stringId, const uint16_t z, const int16_t xOffset, const int8_t* yOffsets, const uint16_t colour)
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

        const auto& vpPos = Map::gameToScreen(Map::Pos3(getSpritePosition(), z), currentRotation);
        psString->vpPos.x = vpPos.x + xOffset;
        psString->vpPos.y = vpPos.y;

        attachStringStruct(*psString);
        return psString;
    }

    // 0x004FD130
    PaintStruct* PaintSession::addToPlotListAsParent(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxSize)
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

    static constexpr Map::Pos3 rotateBoundBoxSize(const Map::Pos3& bbSize, const uint8_t rotation)
    {
        auto output = bbSize;
        // This probably rotates the variables so they're relative to rotation 0.
        // Uses same rotations as directionFlipXAxis
        switch (rotation)
        {
            case 0:
                output.x--;
                output.y--;
                output = Map::Pos3{ Math::Vector::rotate(output, 0), output.z };
                break;
            case 1:
                output.x--;
                output = Map::Pos3{ Math::Vector::rotate(output, 3), output.z };
                break;
            case 2:
                output = Map::Pos3{ Math::Vector::rotate(output, 2), output.z };
                break;
            case 3:
                output.y--;
                output = Map::Pos3{ Math::Vector::rotate(output, 1), output.z };
                break;
        }
        return output;
    }

    // 0x004FD140
    PaintStruct* PaintSession::addToPlotListAsParent(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
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

    // 0x004FD150
    void PaintSession::addToPlotList4FD150(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId.toUInt32();
        // regs.al = offset.x;
        // regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        addr<0xE3F0A0, int16_t>() = boundBoxOffset.x;
        addr<0xE3F0A2, int16_t>() = boundBoxOffset.y;
        addr<0xE3F0A4, uint16_t>() = boundBoxOffset.z;

        call(_4FD150[currentRotation], regs);
    }

    // 0x004FD200
    void PaintSession::addToPlotList4FD200(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId.toUInt32();
        regs.al = offset.x;
        regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        addr<0xE3F0A0, int16_t>() = boundBoxOffset.x;
        addr<0xE3F0A2, int16_t>() = boundBoxOffset.y;
        addr<0xE3F0A4, uint16_t>() = boundBoxOffset.z;
        // Similar to addToPlotListAsParent but shrinks the bound box based on the rt
        call(_4FD200[currentRotation], regs);
    }

    // 0x004FD1E0
    PaintStruct* PaintSession::addToPlotListAsChild(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
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

    void PaintSession::addToPlotList4FD180(ImageId imageId, uint32_t ecx, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId.toUInt32();
        regs.ecx = ecx;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        addr<0xE3F0A0, int16_t>() = boundBoxOffset.x;
        addr<0xE3F0A2, int16_t>() = boundBoxOffset.y;
        addr<0xE3F0A4, uint16_t>() = boundBoxOffset.z;

        call(_4FD180[currentRotation], regs);
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
        _quadrantBackIndex = -1;
        _quadrantFrontIndex = 0;
        _lastPaintString = 0;
        _paintStringHead = 0;

        _viewFlags = options.viewFlags;
        currentRotation = options.rotation;
        addr<0x00E3F0A6, int16_t>() = options.foregroundCullHeight;
    }

    // 0x0045A6CA
    PaintSession* allocateSession(Gfx::RenderTarget& rt, const SessionOptions& options)
    {
        _session.init(rt, options);
        return &_session;
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
        registerTrackHooks();
    }

    const uint16_t segmentOffsets[9] = { Segment::_58, Segment::_5C, Segment::_60, Segment::_64, Segment::_68, Segment::_6C, Segment::_70, Segment::_74, Segment::_78 };

    void PaintSession::setSegmentSupportHeight(const uint16_t segments, const uint16_t height, const uint8_t slope)
    {
        for (int32_t s = 0; s < 9; s++)
        {
            if (segments & segmentOffsets[s])
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

    struct GenerationParameters
    {
        Map::Pos2 mapLoc;
        uint16_t numVerticalQuadrants;
        std::array<Map::Pos2, 5> additionalQuadrants;
        Map::Pos2 nextVerticalQuadrant;
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
        constexpr std::array<Map::Pos2, 5> additionalQuadrants = {
            Math::Vector::rotate(Map::Pos2{ -32, 32 }, direction),
            Math::Vector::rotate(Map::Pos2{ 0, 32 }, direction),
            Math::Vector::rotate(Map::Pos2{ 32, 0 }, direction),
            Math::Vector::rotate(Map::Pos2{ 32, -32 }, direction),
            Math::Vector::rotate(Map::Pos2{ -32, 64 }, direction),
        };
        constexpr auto nextVerticalQuadrant = Math::Vector::rotate(Map::Pos2{ 32, 32 }, direction);

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

    PaintStruct* PaintSession::createNormalPaintStruct(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        auto* const g1 = Gfx::getG1Element(imageId.getIndex());
        if (g1 == nullptr)
        {
            return nullptr;
        }

        const auto swappedRotation = directionFlipXAxis(currentRotation);
        auto swappedRotCoord = Map::Pos3{ Math::Vector::rotate(offset, swappedRotation), offset.z };
        swappedRotCoord += Map::Pos3{ getSpritePosition(), 0 };

        const auto vpPos = Map::gameToScreen(swappedRotCoord, currentRotation);

        if (!imageWithinRT(vpPos, *g1, **_renderTarget))
        {
            return nullptr;
        }

        const auto rotBoundBoxOffset = Map::Pos3{ Math::Vector::rotate(boundBoxOffset, swappedRotation), boundBoxOffset.z };
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
        ps->flags = 0;
        ps->attachedPS = nullptr;
        ps->children = nullptr;
        ps->type = _itemType;
        ps->modId = _trackModId;
        ps->mapPos = _mapPosition;
        ps->tileElement = reinterpret_cast<Map::TileElement*>(*_currentItem);
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
    static bool checkBoundingBox(const PaintStructBoundBox& initialBBox, const PaintStructBoundBox& currentBBox)
    {
        return false;
    }

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
    static PaintStruct* arrangeStructsHelperRotation(PaintStruct* psNext, const uint16_t quadrantIndex, const uint8_t flag)
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
                if (psNext->quadrantFlags & QuadrantFlags::outsideQuadrant)
                {
                    // Reached point outside of specified quadrant.
                    return psQuadrantEntry;
                }
                if (psNext->quadrantFlags & QuadrantFlags::pendingVisit)
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
                if (psNext->quadrantFlags & QuadrantFlags::outsideQuadrant)
                    break;
                if (!(psNext->quadrantFlags & QuadrantFlags::neighbour))
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

    static PaintStruct* arrangeStructsHelper(PaintStruct* psNext, uint16_t quadrantIndex, uint8_t flag, uint8_t rotation)
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
            psCache = arrangeStructsHelper(psCache, quadrantIndex & 0xFFFF, 0, currentRotation);
        }
    }

    // 0x0045EA23
    void PaintSession::drawStructs()
    {
        call(0x0045EA23);
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

        Gfx::setCurrentFontSpriteBase(zoom == 0 ? Font::medium_bold : Font::small);

        char buffer[512]{};

        for (; psString != nullptr; psString = psString->next)
        {
            Ui::Point loc(psString->vpPos.x >> zoom, psString->vpPos.y >> zoom);
            StringManager::formatString(buffer, psString->stringId, psString->args);

            Ui::WindowManager::setWindowColours(0, AdvancedColour(static_cast<Colour>(psString->colour)));
            Ui::WindowManager::setWindowColours(1, AdvancedColour(static_cast<Colour>(psString->colour)));

            Gfx::drawStringYOffsets(unZoomedRt, loc, Colour::black, buffer, psString->yOffsets);
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
    static bool isPSSpriteTypeInFilter(const InteractionItem spriteType, uint32_t filter)
    {
        constexpr uint32_t interactionItemToFilter[] = { 0,
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
                                                         0,
                                                         InteractionItemFlags::building,
                                                         InteractionItemFlags::industry,
                                                         InteractionItemFlags::headquarterBuilding };

        if (spriteType == InteractionItem::noInteraction
            || spriteType == InteractionItem::bridge) // 18 as a type seems to not exist.
            return false;

        uint32_t mask = interactionItemToFilter[static_cast<size_t>(spriteType)];

        if (filter & mask)
        {
            return false;
        }

        return true;
    }

    // 0x0045ED91
    [[nodiscard]] InteractionArg PaintSession::getNormalInteractionInfo(const uint32_t flags)
    {
        InteractionArg info{};

        for (auto* ps = (*_paintHead)->basic.nextQuadrantPS; ps != nullptr; ps = ps->nextQuadrantPS)
        {
            auto* tempPS = ps;
            auto* nextPS = ps;
            while (nextPS != nullptr)
            {
                ps = nextPS;
                if (isSpriteInteractedWith(getRenderTarget(), ps->imageId, ps->vpPos))
                {
                    if (isPSSpriteTypeInFilter(ps->type, flags))
                    {
                        info = { *ps };
                    }
                }
                nextPS = ps->children;
            }

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

            ps = tempPS;
        }
        return info;
    }

    // 0x0048DDE4
    [[nodiscard]] InteractionArg PaintSession::getStationNameInteractionInfo(const uint32_t flags)
    {
        InteractionArg interaction{};

        if (flags & InteractionItemFlags::stationLabel)
        {
            return interaction;
        }

        auto rect = (*_renderTarget)->getDrawableRect();

        for (auto& station : StationManager::stations())
        {
            if (station.flags & StationFlags::flag_5)
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
    [[nodiscard]] InteractionArg PaintSession::getTownNameInteractionInfo(const uint32_t flags)
    {
        InteractionArg interaction{};

        if (flags & InteractionItemFlags::townLabel)
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
}
