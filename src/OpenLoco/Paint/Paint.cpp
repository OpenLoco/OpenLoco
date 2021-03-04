#include "Paint.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Ptr.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "PaintEntity.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    PaintSession _session;

    void PaintSession::setEntityPosition(const Map::map_pos& pos)
    {
        _spritePositionX = pos.x;
        _spritePositionY = pos.y;
    }

    loco_global<int32_t[4], 0x4FD120> _4FD120;
    loco_global<int32_t[4], 0x4FD130> _4FD130;
    loco_global<int32_t[4], 0x4FD140> _4FD140;
    loco_global<int32_t[4], 0x4FD200> _4FD200;

    // 0x004FD120
    void PaintSession::addToStringPlotList(uint32_t amount, string_id stringId, uint16_t y, uint16_t z, const int8_t* y_offsets, int16_t offset_x)
    {
        registers regs;
        regs.bx = stringId;
        regs.edi = ToInt(y_offsets);
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
        regs.edi = ToInt(y_offsets);
        regs.si = offset_x;
        regs.eax = amount;
        regs.cx = y;
        regs.dx = z;
        addr<0xE3F0A8, uint16_t>() = colour;

        call(_4FD120[currentRotation], regs);
    }

    // 0x004FD130
    void PaintSession::addToPlotListAsParent(uint32_t imageId, const Map::map_pos3& offset, const Map::map_pos3& boundBoxSize)
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
    void PaintSession::addToPlotListAsParent(uint32_t imageId, const Map::map_pos3& offset, const Map::map_pos3& boundBoxOffset, const Map::map_pos3& boundBoxSize)
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
    void PaintSession::addToPlotList4FD200(uint32_t imageId, const Map::map_pos3& offset, const Map::map_pos3& boundBoxOffset, const Map::map_pos3& boundBoxSize)
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
    // 0x0045E779
    void PaintSession::attachToPrevious(uint32_t imageId, const Map::map_pos& offset)
    {
        registers regs;
        regs.ebx = imageId;
        regs.ax = offset.x;
        regs.cx = offset.y;

        call(0x0045E779, regs);
    }

    void PaintSession::init(Gfx::drawpixelinfo_t& dpi, const uint16_t viewportFlags)
    {
        _dpi = &dpi;
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
    PaintSession* allocateSession(Gfx::drawpixelinfo_t& dpi, uint16_t viewportFlags)
    {
        _session.init(dpi, viewportFlags);
        return &_session;
    }

    void registerHooks()
    {
        registerHook(
            0x004622A2,
            [](registers& regs) -> uint8_t {
                registers backup = regs;

                PaintSession session;
                session.generate();

                regs = backup;
                return 0;
            });
    }

    // 0x00461CF8
    static void paintTileElements(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x00461CF8, regs);
    }

    // 0x004617C6
    static void paintTileElements2(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x004617C6, regs);
    }

    struct GenerationParameters
    {
        Map::map_pos mapLoc;
        uint16_t numVerticalQuadrants;
        std::array<Map::map_pos, 5> additionalQuadrants;
        Map::map_pos nextVerticalQuadrant;
    };

    template<uint8_t rotation>
    GenerationParameters generateParameters(Gfx::drawpixelinfo_t* context)
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
        constexpr std::array<Map::map_pos, 5> additionalQuadrants = {
            Map::map_pos{ -32, 32 }.rotate(rotOrder[rotation]),
            Map::map_pos{ 0, 32 }.rotate(rotOrder[rotation]),
            Map::map_pos{ 32, 0 }.rotate(rotOrder[rotation]),
            Map::map_pos{ 32, -32 }.rotate(rotOrder[rotation]),
            Map::map_pos{ -32, 64 }.rotate(rotOrder[rotation]),
        };
        constexpr auto nextVerticalQuadrant = Map::map_pos{ 32, 32 }.rotate(rotOrder[rotation]);

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
        if ((addr<0x00525E28, uint32_t>() & (1 << 0)) == 0)
            return;

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
        do
        {
            ps = psNext;
            psNext = psNext->nextQuadrantPS;
            if (psNext == nullptr)
                return ps;
        } while (quadrantIndex > psNext->quadrantIndex);

        // Cache the last visited node so we don't have to walk the whole list again
        auto* psCache = ps;
        auto* psTemp = ps;
        do
        {
            ps = ps->nextQuadrantPS;
            if (ps == nullptr)
                break;

            if (ps->quadrantIndex > quadrantIndex + 1)
            {
                ps->quadrantFlags = QuadrantFlags::bigger;
            }
            else if (ps->quadrantIndex == quadrantIndex + 1)
            {
                ps->quadrantFlags = QuadrantFlags::next | QuadrantFlags::identical;
            }
            else if (ps->quadrantIndex == quadrantIndex)
            {
                ps->quadrantFlags = flag | QuadrantFlags::identical;
            }
        } while (ps->quadrantIndex <= quadrantIndex + 1);
        ps = psTemp;

        while (true)
        {
            while (true)
            {
                psNext = ps->nextQuadrantPS;
                if (psNext == nullptr)
                    return psCache;
                if (psNext->quadrantFlags & QuadrantFlags::bigger)
                    return psCache;
                if (psNext->quadrantFlags & QuadrantFlags::identical)
                    break;
                ps = psNext;
            }

            psNext->quadrantFlags &= ~QuadrantFlags::identical;
            psTemp = ps;

            const PaintStructBoundBox& initialBBox = psNext->bounds;

            while (true)
            {
                ps = psNext;
                psNext = psNext->nextQuadrantPS;
                if (psNext == nullptr)
                    break;
                if (psNext->quadrantFlags & QuadrantFlags::bigger)
                    break;
                if (!(psNext->quadrantFlags & QuadrantFlags::next))
                    continue;

                const PaintStructBoundBox& currentBBox = psNext->bounds;

                const bool compareResult = checkBoundingBox<_TRotation>(initialBBox, currentBBox);

                if (compareResult)
                {
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
            &(*_paintHead)->basic, _quadrantBackIndex & 0xFFFF, QuadrantFlags::next, currentRotation);

        quadrantIndex = _quadrantBackIndex;
        while (++quadrantIndex < _quadrantFrontIndex)
        {
            psCache = arrangeStructsHelper(psCache, quadrantIndex & 0xFFFF, 0, currentRotation);
        }
    }

    static bool isSpriteInteractedWithPaletteSet(Gfx::drawpixelinfo_t* dpi, uint32_t imageId, const Gfx::point_t& coords, const Gfx::PaletteMap& paletteMap)
    {
        static loco_global<const uint8_t*, 0x0050B860> _paletteMap;
        static loco_global<bool, 0x00E40114> _interactionResult;
        _paletteMap = paletteMap.data();
        registers regs{};
        regs.ebx = imageId;
        regs.edi = ToInt(dpi);
        regs.cx = coords.x;
        regs.dx = coords.y;
        call(0x00447A5F, regs);
        return _interactionResult;
    }

    // 0x00447A0E
    static bool isSpriteInteractedWith(Gfx::drawpixelinfo_t* dpi, uint32_t imageId, const Gfx::point_t& coords)
    {
        static loco_global<bool, 0x00E40114> _interactionResult;
        static loco_global<uint32_t, 0x00E04324> _interactionFlags;
        _interactionResult = false;
        auto paletteMap = Gfx::PaletteMap::getDefault();
        imageId &= ~Gfx::ImageIdFlags::translucent;
        if (imageId & Gfx::ImageIdFlags::remap)
        {
            _interactionFlags = Gfx::ImageIdFlags::remap;
            int32_t index = (imageId >> 19) & 0x7F;
            if (imageId & Gfx::ImageIdFlags::remap2)
            {
                index &= 0x1F;
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
        return isSpriteInteractedWithPaletteSet(dpi, imageId, coords, paletteMap);
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

        auto rect = (*_dpi)->getDrawableRect();

        for (auto& station : StationManager::stations())
        {
            if (station.empty())
            {
                continue;
            }

            if (station.flags & station_flags::flag_5)
            {
                continue;
            }

            if (!station.labelPosition.contains(rect, (*_dpi)->zoom_level))
            {
                continue;
            }

            interaction.type = InteractionItem::stationLabel;
            interaction.value = station.id();
            interaction.x = station.x;
            interaction.y = station.y;
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

        auto rect = (*_dpi)->getDrawableRect();

        for (auto& town : TownManager::towns())
        {
            if (town.empty())
            {
                continue;
            }

            if (!town.labelPosition.contains(rect, (*_dpi)->zoom_level))
            {
                continue;
            }

            interaction.type = InteractionItem::townLabel;
            interaction.value = town.id();
            interaction.x = town.x;
            interaction.y = town.y;
        }
        return interaction;
    }
}
