#include "PaintTile.h"
#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Map/TileManager.h"
#include "../Objects/TreeObject.h"
#include "../Ui.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    // 0x004621FF
    static void paintVoid(PaintSession& session, const Map::Pos2& loc)
    {
        constexpr Map::Pos2 unkOffsets[4] = {
            { 0, 0 },
            { 32, 0 },
            { 32, 32 },
            { 0, 32 },
        };

        const auto loc2 = loc + unkOffsets[session.getRotation()];
        const auto vpPos = Map::gameToScreen(Map::Pos3(loc2.x, loc2.y, 16), session.getRotation());
        if (vpPos.y + 32 <= session.getContext()->y)
        {
            return;
        }
        if (vpPos.y - 20 >= session.getContext()->height + session.getContext()->y)
        {
            return;
        }

        session.setEntityPosition(loc2);
        session.setItemType(InteractionItem::noInteraction);
        session.addToPlotListAsParent(ImageIds::blank_tile, { 0, 0, 16 }, { 32, 32, 255 });
    }

    // 0x00461EA7
    static void paintConstructionArrow(PaintSession& session, const Map::Pos2& loc)
    {
        static loco_global<Map::Pos3, 0x00F24942> _constructionArrowLocation;
        static loco_global<uint8_t, 0x00F24948> _constructionArrowDirection;
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::unk_02))
        {
            return;
        }
        if (session.getUnkPosition() != _constructionArrowLocation)
        {
            return;
        }
        session.setEntityPosition(loc);
        const auto dirIndex = (_constructionArrowDirection & 0xFC) | (((_constructionArrowDirection & 0x3) + session.getRotation()) & 0x3);
        constexpr std::array<uint32_t, 16> constructionArrowImages = {
            ImageIds::construction_arrow_north,
            ImageIds::construction_arrow_east,
            ImageIds::construction_arrow_south,
            ImageIds::construction_arrow_west,
            ImageIds::construction_arrow_north2,
            ImageIds::construction_arrow_east2,
            ImageIds::construction_arrow_south2,
            ImageIds::construction_arrow_west2,
            ImageIds::construction_arrow_north3,
            ImageIds::construction_arrow_east3,
            ImageIds::construction_arrow_south3,
            ImageIds::construction_arrow_west3,
            ImageIds::construction_arrow_north_east,
            ImageIds::construction_arrow_south_east,
            ImageIds::construction_arrow_south_west,
            ImageIds::construction_arrow_north_west,
        };
        const auto imageId = Gfx::recolour(constructionArrowImages[dirIndex], Colour::dark_olive_green);
        session.setItemType(InteractionItem::noInteraction);
        session.addToPlotListAsParent(imageId, { 0, 0, _constructionArrowLocation->z }, Map::Pos3(0, 0, _constructionArrowLocation->z + 10), { 32, 32, 255 });
    }

    // 0x004792E7 streetlights?
    static void sub_4792E7(PaintSession& session)
    {
        call(0x004792E7);
    }

    // 0x0046748F
    static void sub_46748F(PaintSession& session)
    {
        call(0x0046748F);
    }

    // 0x0045CA67
    static void sub_45CA67(PaintSession& session)
    {
        call(0x0045CA67);
    }

    // 0x0045CC1B
    static void sub_45CC1B(PaintSession& session)
    {
        call(0x0045CC1B);
    }

    // 0x0042AC9C
    static bool sub_42AC9C(PaintSession& session)
    {
        registers regs;
        call(0x0042AC9C, regs);
        return regs.al != 0;
    }

    // 0x004656BF
    static void paintSurface(PaintSession& session, Map::SurfaceElement& elSurface)
    {
        registers regs;
        regs.esi = X86Pointer(&elSurface);
        regs.dx = elSurface.baseZ() * 4;
        call(0x004656BF, regs);
    }

    // 0x0049B6BF
    static void paintTrack(PaintSession& session, Map::TrackElement& elTrack)
    {
        registers regs;
        regs.esi = X86Pointer(&elTrack);
        regs.ecx = (session.getRotation() + elTrack.unkDirection()) & 0x3;
        regs.dx = elTrack.baseZ() * 4;
        call(0x0049B6BF, regs);
    }

    // 0x0048B313
    static void paintStation(PaintSession& session, Map::StationElement& elStation)
    {
        registers regs;
        regs.esi = X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseZ() * 4;
        call(0x0048B313, regs);
    }

    // 0x0048864C
    static void paintSignal(PaintSession& session, Map::SignalElement& elSignal)
    {
        registers regs;
        regs.esi = X86Pointer(&elSignal);
        regs.ecx = (session.getRotation() + (elSignal.data()[0] & 0x3)) & 0x3;
        regs.dx = elSignal.baseZ() * 4;
        call(0x0048864C, regs);
    }

    // 0x0042C6C4
    static void paintBuilding(PaintSession& session, Map::BuildingElement& elBuilding)
    {
        registers regs;
        regs.esi = X86Pointer(&elBuilding);
        regs.ecx = (session.getRotation() + (elBuilding.data()[0] & 0x3)) & 0x3;
        regs.dx = elBuilding.baseZ() * 4;
        call(0x0042C6C4, regs);
    }

    constexpr std::array<uint8_t, 6> _50076A = { 3, 0, 1, 2, 1, 4 };
    constexpr std::array<bool, 6> _500770 = { true, true, false, false, true, true };
    constexpr std::array<Map::Pos2, 4> _50074C = {
        Map::Pos2{ 7, 7 },
        Map::Pos2{ 7, 23 },
        Map::Pos2{ 23, 23 },
        Map::Pos2{ 23, 7 },
    };
    // 0x004BAEDA
    static void paintTree(PaintSession& session, Map::TreeElement& elTree)
    {
        session.setItemType(InteractionItem::tree);

        auto* treeObj = ObjectManager::get<TreeObject>(elTree.treeObjectId());
        const uint8_t viewableRotation = (session.getRotation() + elTree.rotation()) & 0x3;
        uint32_t _imageId1 = (viewableRotation % treeObj->num_rotations) + elTree.unk5l() * treeObj->num_rotations;
        uint32_t _imageId2 = 0;

        const uint8_t al = elTree.unk7l();
        uint8_t season = elTree.season();

        bool hasImage2 = false;

        if (al != 7)
        {
            hasImage2 = true;

            uint32_t edx = (elTree.unk7l()) + 1;

            // TODO: Make bool list
            if (_500770[season])
            {
                season = _50076A[season];
                edx = (-edx) & 0b111;
            }

            const uint8_t altSeason = elTree.isAltSeason() ? 1 : 0;

            edx = edx << 26;
            _imageId2 = edx | (_imageId1 + treeObj->sprites[altSeason][season]);
        }

        uint32_t _shadowImageId = 0;
        if (treeObj->flags & TreeObjectFlags::unk7)
        {
            _shadowImageId = 0x41900000 | (treeObj->shadowImageOffset + _imageId1);
        }

        const int16_t height = elTree.baseZ() * 4;
        const int ecx = (elTree.unk0u() + session.getRotation()) % 4;
        const auto imageOffset = Map::Pos3(_50074C[ecx].x, _50074C[ecx].y, height);

        const int16_t boundBoxOffsetZ = std::min(elTree.clearZ() - elTree.baseZ(), 0x20) * 4 - 3;

        const uint8_t altSeason = elTree.isAltSeason() ? 1 : 0;

        _imageId1 = _imageId1 + treeObj->sprites[altSeason][ecx];

        if (treeObj->colours != 0)
        {
            // No vanilla object has this property set
            uint8_t colour = elTree.colour();
            _imageId2 = Gfx::recolour(_imageId2, colour);
            _imageId1 = Gfx::recolour(_imageId1, colour);
        }

        if (elTree.isGhost())
        {
            session.setItemType(InteractionItem::noInteraction);
            _imageId2 = applyGhostToImage(_imageId2);
            _imageId1 = applyGhostToImage(_imageId1);
        }

        if (_shadowImageId != 0)
        {
            if (session.getContext()->zoom_level <= 1)
            {
                session.addToPlotListAsParent(_shadowImageId, imageOffset, { 18, 18, 1 }, imageOffset);
            }
        }

        session.addToPlotListAsParent(_imageId1, imageOffset, { 2, 2, boundBoxOffsetZ }, imageOffset + Map::Pos3(0, 0, 2));

        if (hasImage2)
        {
            session.addToPlotList4FD1E0(_imageId2, imageOffset, { 2, 2, boundBoxOffsetZ }, imageOffset + Map::Pos3(0, 0, 2));
        }
    }

    // 0x004C3D7C
    static void paintWall(PaintSession& session, Map::WallElement& elWall)
    {
        registers regs;
        regs.esi = X86Pointer(&elWall);
        regs.ecx = (session.getRotation() + (elWall.data()[0] & 0x3)) & 0x3;
        regs.dx = elWall.baseZ() * 4;
        call(0x004C3D7C, regs);
    }

    // 0x004759A6
    static void paintRoad(PaintSession& session, Map::RoadElement& elRoad)
    {
        registers regs;
        regs.esi = X86Pointer(&elRoad);
        regs.ecx = (session.getRotation() + elRoad.unkDirection()) & 0x3;
        regs.dx = elRoad.baseZ() * 4;
        call(0x004759A6, regs);
    }

    // 0x00453C52
    static void paintIndustry(PaintSession& session, Map::IndustryElement& elIndustry)
    {
        registers regs;
        regs.esi = X86Pointer(&elIndustry);
        regs.ecx = (session.getRotation() + (elIndustry.data()[0] & 0x3)) & 0x3;
        regs.dx = elIndustry.baseZ() * 4;
        call(0x00453C52, regs);
    }

    // 0x00461CF8
    void paintTileElements(PaintSession& session, const Map::Pos2& loc)
    {
        if (loc.x >= 383 * Map::tile_size || loc.y >= 383 * Map::tile_size || loc.x < 32 || loc.y < 32)
        {
            paintVoid(session, loc);
            return;
        }

        session.setSegmentSupportHeight(Segment::all, std::numeric_limits<uint16_t>::max(), 0);
        session.setGeneralSupportHeight(std::numeric_limits<uint16_t>::max(), 0);
        session.resetTunnels();
        session.setUnkPosition(loc);
        session.setMapPosition(loc);

        auto tile = Map::TileManager::get(loc);
        uint8_t maxClearZ = 0;
        for (const auto& el : tile)
        {
            maxClearZ = std::max(maxClearZ, el.clearZ());
            const auto* surface = el.asSurface();
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
        session.setMaxHeight((maxClearZ * 4) + 32);

        constexpr Map::Pos2 unkOffsets[4] = {
            { 0, 0 },
            { 32, 0 },
            { 32, 32 },
            { 0, 32 },
        };

        const auto loc2 = loc + unkOffsets[session.getRotation()];
        const auto vpPos = Map::gameToScreen(Map::Pos3(loc2.x, loc2.y, 0), session.getRotation());
        paintConstructionArrow(session, loc2);

        if (vpPos.y + 52 <= session.getContext()->y)
        {
            return;
        }
        if (vpPos.y - session.getMaxHeight() > session.getContext()->y + session.getContext()->height)
        {
            return;
        }

        session.setEntityPosition(loc2);
        session.resetTileColumn({ vpPos.x, vpPos.y });

        for (auto& el : tile)
        {
            // This is not a good idea not all tiles (surface) have a rotation
            //const uint8_t rotation = (el.rotation() + session.getRotation()) & 0x3;
            session.setUnkVpY(vpPos.y - el.baseZ() * 4);
            session.setCurrentItem(&el);
            switch (el.type())
            {
                case Map::ElementType::surface:
                {
                    auto* elSurface = el.asSurface();
                    if (elSurface != nullptr)
                    {
                        paintSurface(session, *elSurface);
                    }
                    break;
                }
                case Map::ElementType::track:
                {
                    auto* elTrack = el.asTrack();
                    if (elTrack != nullptr)
                    {
                        paintTrack(session, *elTrack);
                    }
                    break;
                }
                case Map::ElementType::station:
                {
                    auto* elStation = el.asStation();
                    if (elStation != nullptr)
                    {
                        paintStation(session, *elStation);
                    }
                    break;
                }
                case Map::ElementType::signal:
                {
                    auto* elSignal = el.asSignal();
                    if (elSignal != nullptr)
                    {
                        paintSignal(session, *elSignal);
                    }
                    break;
                }
                case Map::ElementType::building:
                {
                    auto* elBuilding = el.asBuilding();
                    if (elBuilding != nullptr)
                    {
                        paintBuilding(session, *elBuilding);
                    }
                    break;
                }
                case Map::ElementType::tree:
                {
                    auto* elTree = el.asTree();
                    if (elTree != nullptr)
                    {
                        paintTree(session, *elTree);
                    }
                    break;
                }
                case Map::ElementType::wall:
                {
                    auto* elWall = el.asWall();
                    if (elWall != nullptr)
                    {
                        paintWall(session, *elWall);
                    }
                    break;
                }
                case Map::ElementType::road:
                {
                    auto* elRoad = el.asRoad();
                    if (elRoad != nullptr)
                    {
                        paintRoad(session, *elRoad);
                    }
                    break;
                }
                case Map::ElementType::industry:
                {
                    auto* elIndustry = el.asIndustry();
                    if (elIndustry != nullptr)
                    {
                        paintIndustry(session, *elIndustry);
                    }
                    break;
                }
            }
            if (el.isLast() || el.baseZ() != ((&el) + 1)->baseZ())
            {
                if (session.get112C300() != 0)
                {
                    sub_4792E7(session);
                }
                if (session.getF003F4() != 0)
                {
                    sub_46748F(session);
                }

                sub_45CA67(session);
                sub_45CC1B(session);
                session.setF003F6(0);
                if (session.get525CE4(0) != 0xFFFF)
                {
                    if (sub_42AC9C(session))
                    {
                        session.setSegmentSupportHeight(Segment::all, 0xFFFF, 0);
                    }
                    if (session.getGeneralSupportHeight().height >= session.get525CE4(0))
                    {
                        session.setGeneralSupportHeight(session.get525CE4(0), 0x20);
                    }
                    session.set525CE4(0, 0xFFFF);
                    session.set525CF0(0);
                }

                if (session.get525CF8() != 0)
                {
                    for (auto bit = Utility::bitScanForward(session.get525CF8()); bit != -1; bit = Utility::bitScanForward(session.get525CF8()))
                    {
                        session.set525CF8(session.get525CF8() & ~(1 << bit));
                        session.setSegmentSupportHeight(1 << bit, 0xFFFF, 0);
                    }
                }
            }
        }
    }

    // 0x004617C6
    void paintTileElements2(PaintSession& session, const Map::Pos2& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x004617C6, regs);
    }
}
