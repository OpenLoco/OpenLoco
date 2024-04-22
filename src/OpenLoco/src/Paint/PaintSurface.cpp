#include "PaintSurface.h"
#include "Graphics/RenderTarget.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Paint.h"
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
        uint8_t landObjId = elSurface.terrain();
        if (elSurface.isIndustrial())
        {
            auto* industry = IndustryManager::get(elSurface.industryId());
            // 0x00F252A8
            auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);
        }

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
    }
}
