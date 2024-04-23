#include "PaintSurface.h"
#include "Graphics/RenderTarget.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "Ui/ViewportInteraction.h"
#include "World/IndustryManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::Paint
{
    struct CornerHeight
    {
        uint8_t top;
        uint8_t right;
        uint8_t bottom;
        uint8_t left;
    };

    struct TileDescriptor
    {
        World::Pos2 pos;
        const World::SurfaceElement* elSurface;
        const LandObject* landObject;
        uint8_t slope;
        CornerHeight cornerHeights;
    };

    // TODO: Check
    static constexpr std::array<CornerHeight, 32> cornerHeights = {
        // T  R  B  L
        CornerHeight{ 0, 0, 0, 0 },
        CornerHeight{ 0, 0, 1, 0 },
        CornerHeight{ 0, 0, 0, 1 },
        CornerHeight{ 0, 0, 1, 1 },
        CornerHeight{ 1, 0, 0, 0 },
        CornerHeight{ 1, 0, 1, 0 },
        CornerHeight{ 1, 0, 0, 1 },
        CornerHeight{ 1, 0, 1, 1 },
        CornerHeight{ 0, 1, 0, 0 },
        CornerHeight{ 0, 1, 1, 0 },
        CornerHeight{ 0, 1, 0, 1 },
        CornerHeight{ 0, 1, 1, 1 },
        CornerHeight{ 1, 1, 0, 0 },
        CornerHeight{ 1, 1, 1, 0 },
        CornerHeight{ 1, 1, 0, 1 },
        CornerHeight{ 1, 1, 1, 1 },
        CornerHeight{ 0, 0, 0, 0 },
        CornerHeight{ 0, 0, 1, 0 },
        CornerHeight{ 0, 0, 0, 1 },
        CornerHeight{ 0, 0, 1, 1 },
        CornerHeight{ 1, 0, 0, 0 },
        CornerHeight{ 1, 0, 1, 0 },
        CornerHeight{ 1, 0, 0, 1 },
        CornerHeight{ 1, 0, 1, 2 },
        CornerHeight{ 0, 1, 0, 0 },
        CornerHeight{ 0, 1, 1, 0 },
        CornerHeight{ 0, 1, 0, 1 },
        CornerHeight{ 0, 1, 2, 1 },
        CornerHeight{ 1, 1, 0, 0 },
        CornerHeight{ 1, 2, 1, 0 },
        CornerHeight{ 2, 1, 0, 1 },
        CornerHeight{ 1, 1, 1, 1 },
    };

    static constexpr std::array<uint8_t, 32> k4FD97E = {
        0,
        2,
        1,
        3,
        8,
        10,
        9,
        11,
        4,
        6,
        5,
        7,
        12,
        14,
        13,
        15,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        17,
        0,
        0,
        0,
        16,
        0,
        18,
        15,
        0,
    };

    static constexpr std::array<uint8_t, 19> k4FD30C = {
        0,
        0,
        0,
        8,
        0,
        0,
        8,
        16,
        0,
        8,
        0,
        16,
        8,
        16,
        16,
        16,
        16,
        16,
        16,
    };

    std::array<std::array<World::Pos2, 4>, 4> _unk4FD99E = {
        std::array<World::Pos2, 4>{
            World::Pos2{ 32, 0 },
            World::Pos2{ -32, 0 },
            World::Pos2{ -64, -32 },
            World::Pos2{ 0, -64 },
        },
        std::array<World::Pos2, 4>{
            World::Pos2{ 0, 32 },
            World::Pos2{ -64, 0 },
            World::Pos2{ -32, -64 },
            World::Pos2{ 32, -32 },
        },
        std::array<World::Pos2, 4>{
            World::Pos2{ 0, -32 },
            World::Pos2{ 0, 0 },
            World::Pos2{ -32, 0 },
            World::Pos2{ -32, -32 },
        },
        std::array<World::Pos2, 4>{
            World::Pos2{ -32, 0 },
            World::Pos2{ -32, -32 },
            World::Pos2{ 0, -32 },
            World::Pos2{ 0, 0 },
        },
    };

    static constexpr uint8_t getRotatedSlope(uint8_t slope, uint8_t rotation)
    {
        return Numerics::rotl4bit(slope & 0xF, rotation) | (slope & 0x10);
    }

    // 0x004656BF
    void paintSurface(PaintSession& session, World::SurfaceElement& elSurface)
    {
        const auto zoomLevel = session.getRenderTarget()->zoomLevel;
        session.setDidPassSurface(true);

        // 0x00F252B0 / 0x00F252B4 but if 0x00F252B0 == -2 that means industrial
        [[maybe_unused]] uint8_t landObjId = elSurface.terrain();

        const auto rotation = session.getRotation();

        const auto rotatedSlope = getRotatedSlope(elSurface.slope(), rotation);

        const auto selfDescriptor = TileDescriptor{
            session.getSpritePosition(),
            &elSurface,
            ObjectManager::get<LandObject>(elSurface.terrain()),
            rotatedSlope,
            {
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].top),
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].right),
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].bottom),
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].left),
            },
        };
        std::array<TileDescriptor, 4> tileDescriptors;

        for (std::size_t i = 0; i < std::size(tileDescriptors); i++)
        {
            const auto& offset = _unk4FD99E[i][rotation];
            const auto position = session.getSpritePosition() + offset;

            TileDescriptor& descriptor = tileDescriptors[i];

            descriptor.elSurface = nullptr;
            if (!World::validCoords(position))
            {
                continue;
            }

            descriptor.elSurface = World::TileManager::get(position).surface();
            if (descriptor.elSurface == nullptr)
            {
                continue;
            }

            const uint32_t surfaceSlope = getRotatedSlope(descriptor.elSurface->slope(), rotation);

            const uint8_t baseZ = descriptor.elSurface->baseZ();
            const CornerHeight& ch = cornerHeights[surfaceSlope];

            descriptor.pos = position;
            descriptor.landObject = ObjectManager::get<LandObject>(descriptor.elSurface->terrain());
            descriptor.slope = surfaceSlope;
            descriptor.cornerHeights.top = baseZ + ch.top;
            descriptor.cornerHeights.right = baseZ + ch.right;
            descriptor.cornerHeights.bottom = baseZ + ch.bottom;
            descriptor.cornerHeights.left = baseZ + ch.left;
        }

        if (((session.getViewFlags() & Ui::ViewportFlags::height_marks_on_land) != Ui::ViewportFlags::none)
            && zoomLevel == 0)
        {
            const auto markerPos = session.getUnkPosition() + World::Pos2(16, 16);
            const auto markerHeight = World::TileManager::getHeight(markerPos).landHeight + 3;
            const auto markerImageIndex = getHeightMarkerImage(markerHeight);

            const auto imageId = ImageId{ getHeightMarkerImage(markerImageIndex), Colour::mutedAvocadoGreen };
            const World::Pos3 offset(16, 16, markerHeight);
            const World::Pos3 bbSize(1, 1, 0);
            session.addToPlotListAsParent(imageId, offset, bbSize);
        }

        // 0x00F25314
        [[maybe_unused]] const auto cliffEdgeImageBase = selfDescriptor.landObject->cliffEdgeImage;
        // 0x00F252AC
        const auto unkF252AC = k4FD97E[selfDescriptor.slope];

        if (elSurface.isIndustrial())
        {
            // 0x00465C96
            auto* industry = IndustryManager::get(elSurface.industryId());
            auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);

            session.setItemType(Ui::ViewportInteraction::InteractionItem::industryTree);

            const auto variation = industryObj->var_1A * elSurface.var_6_SLR5() + ((industryObj->var_E9 - 1) & rotation) * 21;

            // Draw trees if they exist
            {
                const auto height = elSurface.baseHeight() + k4FD30C[unkF252AC];
                const auto imageIndex = industryObj->var_16 + variation + (elSurface.snowCoverage() ? 20 : 19);

                const World::Pos3 offset(0, 0, height);
                const World::Pos3 bbOffset(14, 14, height + 4);
                const World::Pos3 bbSize(4, 4, 14);
                session.addToPlotList4FD150(ImageId(imageIndex), offset, bbOffset, bbSize);
            }

            session.setItemType(Ui::ViewportInteraction::InteractionItem::surface);

            if (zoomLevel == 0 && industryObj->hasFlags(IndustryObjectFlags::unk26)
                || elSurface.snowCoverage() == 0)
            {
                // Draw main surface image
                const auto unkImageIndex = industryObj->var_16 + variation + unkF252AC;
                const auto imageId = [unkImageIndex, &session]() {
                    if ((session.getViewFlags() & (Ui::ViewportFlags::underground_view | Ui::ViewportFlags::flag_7)) != Ui::ViewportFlags::none)
                    {
                        return ImageId(unkImageIndex).withTranslucency(ExtColour::unk30);
                    }
                    return ImageId(unkImageIndex);
                }();

                const World::Pos3 offsetUnk(0, 0, elSurface.baseHeight());
                const World::Pos3 bbSizeUnk(32, 32, -1);
                session.addToPlotListAsParent(imageId, offsetUnk, bbSizeUnk);
            }
            else
            {
                // Draw snow surface image
            }
            // 0x00465E92
        }
        else
        {
            // 0x00465BE0
        }
    }
}
