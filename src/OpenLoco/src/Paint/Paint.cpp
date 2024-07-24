#include "Paint.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "PaintEntity.h"
#include "PaintTile.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

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
                // Because one component may be the maximum we add the centre to be a positive value.
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

    // 0x004FD150
    PaintStruct* PaintSession::addToPlotList4FD150(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        // This is identical to addToPlotListAsParent but the offset.x and offset.y are 0
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

    // 0x004FD170
    PaintStruct* PaintSession::addToPlotListTrackRoad(ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        _lastPS = nullptr;

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

    // 0x004FD180
    PaintStruct* PaintSession::addToPlotListTrackRoadAddition(ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        _lastPS = nullptr;

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

    void PaintSession::init(const Gfx::RenderTarget& rt, const SessionOptions& options)
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
        _lastPaintString = nullptr;
        _paintStringHead = nullptr;

        _viewFlags = options.viewFlags;
        currentRotation = options.rotation;

        // TODO: unused
        _foregroundCullingHeight = options.foregroundCullHeight;
    }

    // 0x0045A6CA
    PaintSession* allocateSession(const Gfx::RenderTarget& rt, const SessionOptions& options)
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
    }

    void PaintSession::setSegmentsSupportHeight(const SegmentFlags segments, const uint16_t height, const uint8_t slope)
    {
        for (int32_t s = 0; s < 9; s++)
        {
            if ((segments & kSegmentOffsets[s]) != SegmentFlags::none)
            {
                setSegmentSupportHeight(s, height, slope);
            }
        }
    }

    void PaintSession::setSegmentSupportHeight(const uint8_t segment, const uint16_t height, const uint8_t slope)
    {
        _supportSegments[segment].height = height;
        if (height != 0xFFFF)
        {
            _supportSegments[segment].slope = slope;
            _supportSegments[segment].var_03 = 0;
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
        auto& tunnelCount = _tunnelCounts[edge];
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
    GenerationParameters generateParameters(const Gfx::RenderTarget* rt)
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

    static void drawStruct(const Gfx::RenderTarget& rt, Gfx::DrawingContext& drawingCtx, const PaintStruct& ps, const bool shouldCull)
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
            drawingCtx.drawImageMasked(imagePos, imageId, ps.maskedImageId);
        }
        else
        {
            drawingCtx.drawImage(imagePos, imageId);
        }
    }

    static void drawAttachStruct(const Gfx::RenderTarget& rt, Gfx::DrawingContext& drawingCtx, const PaintStruct& ps, const AttachedPaintStruct& attachPs, const bool shouldCull)
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
            drawingCtx.drawImageMasked(imagePos, imageId, attachPs.maskedImageId);
        }
        else
        {
            drawingCtx.drawImage(imagePos, imageId);
        }
    }

    // 0x0045EA23
    void PaintSession::drawStructs(Gfx::DrawingContext& drawingCtx)
    {
        const Gfx::RenderTarget& rt = drawingCtx.currentRenderTarget();

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
    void PaintSession::drawStringStructs(Gfx::DrawingContext& drawingCtx)
    {
        PaintStringStruct* psString = _paintStringHead;
        if (psString == nullptr)
        {
            return;
        }

        Gfx::RenderTarget unZoomedRt = drawingCtx.currentRenderTarget();
        const auto zoom = unZoomedRt.zoomLevel;

        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= zoom;
        unZoomedRt.y >>= zoom;
        unZoomedRt.width >>= zoom;
        unZoomedRt.height >>= zoom;

        drawingCtx.pushRenderTarget(unZoomedRt);

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.setCurrentFont(zoom == 0 ? Gfx::Font::medium_bold : Gfx::Font::small);

        char buffer[512]{};

        for (; psString != nullptr; psString = psString->next)
        {
            // FIXME: Modify psString->args to be a buffer.
            FormatArguments args{ reinterpret_cast<std::byte*>(psString->args), sizeof(psString->args) };

            Ui::Point loc(psString->vpPos.x >> zoom, psString->vpPos.y >> zoom);
            StringManager::formatString(buffer, psString->stringId, args);

            Ui::WindowManager::setWindowColours(Ui::WindowColour::primary, AdvancedColour(static_cast<Colour>(psString->colour)));
            Ui::WindowManager::setWindowColours(Ui::WindowColour::secondary, AdvancedColour(static_cast<Colour>(psString->colour)));

            tr.drawStringYOffsets(loc, Colour::black, buffer, psString->yOffsets);
        }

        drawingCtx.popRenderTarget();
    }

    // 0x00447C21
    static bool isPixelPresentBMP(
        const ImageId image, const Gfx::G1Element& g1, const Ui::Point& coords, const Gfx::PaletteMap::View paletteMap)
    {
        const auto* index = g1.offset + (coords.y * g1.width) + coords.x;

        // Needs investigation as it has no consideration for pure BMP maps.
        if (!g1.hasFlags(Gfx::G1ElementFlags::hasTransparency))
        {
            return false;
        }

        if (image.hasPrimary())
        {
            return paletteMap[*index] != 0;
        }

        if (image.isBlended())
        {
            return false;
        }

        return (*index != 0);
    }

    // 0x00447D26
    static bool isPixelPresentRLE(const Gfx::G1Element& g1, const Ui::Point& coords)
    {
        const uint16_t* data16 = reinterpret_cast<const uint16_t*>(g1.offset);
        uint16_t startOffset = data16[coords.y];
        const uint8_t* data8 = static_cast<const uint8_t*>(g1.offset) + startOffset;

        bool lastDataLine = false;
        while (!lastDataLine)
        {
            int32_t numPixels = *data8++;
            uint8_t pixelRunStart = *data8++;
            lastDataLine = numPixels & 0x80;
            numPixels &= 0x7F;
            data8 += numPixels;

            if (pixelRunStart <= coords.x && coords.x < pixelRunStart + numPixels)
            {
                return true;
            }
        }
        return false;
    }

    // 0x00447A5F
    static bool isSpriteInteractedWithPaletteSet(const Gfx::RenderTarget* rt, ImageId imageId, const Ui::Point& coords, const Gfx::PaletteMap::View paletteMap)
    {
        const auto* g1 = Gfx::getG1Element(imageId.getIndex());
        if (g1 == nullptr)
        {
            return false;
        }

        auto zoomLevel = rt->zoomLevel;
        Ui::Point interactionPoint{ rt->x, rt->y };
        Ui::Point origin = coords;

        if (zoomLevel > 0)
        {
            if (g1->hasFlags(Gfx::G1ElementFlags::noZoomDraw))
            {
                return false;
            }

            while (g1->hasFlags(Gfx::G1ElementFlags::hasZoomSprites) && zoomLevel > 0)
            {
                imageId = ImageId(imageId.getIndex() - g1->zoomOffset);
                g1 = Gfx::getG1Element(imageId.getIndex());
                if (g1 == nullptr || g1->hasFlags(Gfx::G1ElementFlags::noZoomDraw))
                {
                    return false;
                }
                zoomLevel = zoomLevel - 1;
                interactionPoint.x >>= 1;
                interactionPoint.y >>= 1;
                origin.x >>= 1;
                origin.y >>= 1;
            }
        }

        origin.x += g1->xOffset;
        origin.y += g1->yOffset;
        interactionPoint -= origin;

        if (interactionPoint.x < 0 || interactionPoint.y < 0 || interactionPoint.x >= g1->width || interactionPoint.y >= g1->height)
        {
            return false;
        }

        if (g1->hasFlags(Gfx::G1ElementFlags::isRLECompressed))
        {
            return isPixelPresentRLE(*g1, interactionPoint);
        }

        if (!g1->hasFlags(Gfx::G1ElementFlags::unk1))
        {
            return isPixelPresentBMP(imageId, *g1, interactionPoint, paletteMap);
        }

        return false;
    }

    // 0x00447A0E
    static bool isSpriteInteractedWith(const Gfx::RenderTarget* rt, ImageId imageId, const Ui::Point& coords)
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
        return isSpriteInteractedWithPaletteSet(rt, imageId, coords, paletteMap);
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
