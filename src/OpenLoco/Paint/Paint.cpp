#include "Paint.h"
#include "../Game.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "PaintEntity.h"
#include "PaintTile.h"

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
                maxClearZ = std::max<uint8_t>(maxClearZ, surface->water() * 4);
            }
            if (surface->hasHighTypeFlag())
            {
                maxClearZ = std::max<uint8_t>(maxClearZ, surface->clearZ() + 24);
            }
        }
        _maxHeight = (maxClearZ * 4) + 32;
    }

    loco_global<int32_t[4], 0x4FD120> _4FD120;
    loco_global<int32_t[4], 0x4FD130> _4FD130;
    loco_global<int32_t[4], 0x4FD140> _4FD140;
    loco_global<int32_t[4], 0x4FD1E0> _4FD1E0;
    loco_global<int32_t[4], 0x4FD200> _4FD200;

    // 0x004FD120
    void PaintSession::addToStringPlotList(uint32_t amount, string_id stringId, uint16_t y, uint16_t z, const int8_t* y_offsets, int16_t offset_x)
    {
        registers regs;
        regs.bx = stringId;
        regs.edi = X86Pointer(y_offsets);
        regs.si = offset_x;
        regs.eax = amount;
        regs.cx = y;
        regs.dx = z;

        call(_4FD120[currentRotation], regs);
    }

    // 0x004FD120
    void PaintSession::addToStringPlotList(uint32_t amount, string_id stringId, uint16_t y, uint16_t z, const int8_t* y_offsets, int16_t offset_x, uint16_t colour)
    {
        registers regs;
        regs.bx = stringId;
        regs.edi = X86Pointer(y_offsets);
        regs.si = offset_x;
        regs.eax = amount;
        regs.cx = y;
        regs.dx = z;
        addr<0xE3F0A8, uint16_t>() = colour;

        call(_4FD120[currentRotation], regs);
    }

    // 0x004FD130
    void PaintSession::addToPlotListAsParent(uint32_t imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId;
        regs.al = offset.x;
        regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        call(_4FD130[currentRotation], regs);
    }

    // 0x004FD140
    void PaintSession::addToPlotListAsParent(uint32_t imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId;
        regs.al = offset.x;
        regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        addr<0xE3F0A0, int16_t>() = boundBoxOffset.x;
        addr<0xE3F0A2, int16_t>() = boundBoxOffset.y;
        addr<0xE3F0A4, uint16_t>() = boundBoxOffset.z;

        call(_4FD140[currentRotation], regs);
    }

    // 0x004FD200
    void PaintSession::addToPlotList4FD200(uint32_t imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId;
        regs.al = offset.x;
        regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        addr<0xE3F0A0, int16_t>() = boundBoxOffset.x;
        addr<0xE3F0A2, int16_t>() = boundBoxOffset.y;
        addr<0xE3F0A4, uint16_t>() = boundBoxOffset.z;

        call(_4FD200[currentRotation], regs);
    }

    // 0x004FD1E0
    void PaintSession::addToPlotList4FD1E0(uint32_t imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        registers regs;
        regs.ebx = imageId;
        regs.al = offset.x;
        regs.cl = offset.y;
        regs.dx = offset.z;
        regs.di = boundBoxSize.x;
        regs.si = boundBoxSize.y;
        regs.ah = boundBoxSize.z;

        addr<0xE3F0A0, int16_t>() = boundBoxOffset.x;
        addr<0xE3F0A2, int16_t>() = boundBoxOffset.y;
        addr<0xE3F0A4, uint16_t>() = boundBoxOffset.z;

        call(_4FD1E0[currentRotation], regs);
    }

    // 0x0045E779
    void PaintSession::attachToPrevious(uint32_t imageId, const Map::Pos2& offset)
    {
        registers regs;
        regs.ebx = imageId;
        regs.ax = offset.x;
        regs.cx = offset.y;

        call(0x0045E779, regs);
    }

    void PaintSession::init(Gfx::Context& context, const uint16_t viewportFlags)
    {
        _context = &context;
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
    }

    // 0x0045A6CA
    PaintSession* allocateSession(Gfx::Context& context, uint16_t viewportFlags)
    {
        _session.init(context, viewportFlags);
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
    GenerationParameters generateParameters(Gfx::Context* context)
    {
        // TODO: Work out what these constants represent
        uint16_t numVerticalQuadrants = (context->height + (rotation == 0 ? 1040 : 1056)) >> 5;

        auto mapLoc = Ui::viewportCoordToMapCoord(static_cast<int16_t>(context->x & 0xFFE0), static_cast<int16_t>((context->y - 16) & 0xFFE0), 0, rotation);
        if constexpr (rotation & 1)
        {
            mapLoc.y -= 16;
        }
        mapLoc.x &= 0xFFE0;
        mapLoc.y &= 0xFFE0;

        constexpr uint8_t rotOrder[] = { 0, 3, 2, 1 };

        const auto direction = rotOrder[rotation];
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

    // 0x004622A2
    void PaintSession::generate()
    {
        if (!Game::hasFlags(1u << 0))
            return;

        viewFlags = addr<0x00E3F0BC, uint16_t>();
        currentRotation = Ui::WindowManager::getCurrentRotation();
        switch (currentRotation)
        {
            case 0:
                generateTilesAndEntities(generateParameters<0>(getContext()));
                break;
            case 1:
                generateTilesAndEntities(generateParameters<1>(getContext()));
                break;
            case 2:
                generateTilesAndEntities(generateParameters<2>(getContext()));
                break;
            case 3:
                generateTilesAndEntities(generateParameters<3>(getContext()));
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

    static bool isSpriteInteractedWithPaletteSet(Gfx::Context* context, uint32_t imageId, const Ui::Point& coords, const Gfx::PaletteMap& paletteMap)
    {
        static loco_global<const uint8_t*, 0x0050B860> _paletteMap;
        static loco_global<bool, 0x00E40114> _interactionResult;
        _paletteMap = paletteMap.data();
        registers regs{};
        regs.ebx = imageId;
        regs.edi = X86Pointer(context);
        regs.cx = coords.x;
        regs.dx = coords.y;
        call(0x00447A5F, regs);
        return _interactionResult;
    }

    // 0x00447A0E
    static bool isSpriteInteractedWith(Gfx::Context* context, uint32_t imageId, const Ui::Point& coords)
    {
        static loco_global<bool, 0x00E40114> _interactionResult;
        static loco_global<uint32_t, 0x00E04324> _interactionFlags;
        _interactionResult = false;
        auto paletteMap = Gfx::PaletteMap::getDefault();
        imageId &= ~Gfx::ImageIdFlags::translucent;
        if (imageId & Gfx::ImageIdFlags::remap)
        {
            _interactionFlags = Gfx::ImageIdFlags::remap;
            ExtColour index = static_cast<ExtColour>((imageId >> 19) & 0x7F);
            if (imageId & Gfx::ImageIdFlags::remap2)
            {
                index = static_cast<ExtColour>((imageId >> 19) & 0x1F);
            }
            if (auto pm = Gfx::getPaletteMapForColour(index))
            {
                paletteMap = *pm;
            }
        }
        else
        {
            _interactionFlags = 0;
        }
        return isSpriteInteractedWithPaletteSet(context, imageId, coords, paletteMap);
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
                if (isSpriteInteractedWith(getContext(), ps->imageId, { ps->x, ps->y }))
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
                if (isSpriteInteractedWith(getContext(), attachedPS->imageId, { static_cast<int16_t>(attachedPS->x + ps->x), static_cast<int16_t>(attachedPS->y + ps->y) }))
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

        auto rect = (*_context)->getDrawableRect();

        for (auto& station : StationManager::stations())
        {
            if (station.flags & StationFlags::flag_5)
            {
                continue;
            }

            if (!station.labelFrame.contains(rect, (*_context)->zoom_level))
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

        auto rect = (*_context)->getDrawableRect();

        for (auto& town : TownManager::towns())
        {
            if (!town.labelFrame.contains(rect, (*_context)->zoom_level))
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
