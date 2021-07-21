#include "TileManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Map/Map.hpp"
#include "../ViewportManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::TileManager
{

#pragma pack(push, 1)
    struct TileAnimation
    {
        uint8_t baseZ;
        uint8_t type;
        Map::Pos2 pos;
    };
    static_assert(sizeof(TileAnimation) == 6);
#pragma pack(pop)

    constexpr size_t maxAnimations = 0x2000;

    static loco_global<TileElement*, 0x005230C8> _elements;
    static loco_global<TileElement* [0x30004], 0x00E40134> _tiles;
    static loco_global<TileElement*, 0x00F00134> _elementsEnd;
    static loco_global<coord_t, 0x00F24486> _mapSelectionAX;
    static loco_global<coord_t, 0x00F24488> _mapSelectionBX;
    static loco_global<coord_t, 0x00F2448A> _mapSelectionAY;
    static loco_global<coord_t, 0x00F2448C> _mapSelectionBY;
    static loco_global<uint16_t, 0x00F2448E> _word_F2448E;
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;
    static loco_global<uint16_t, 0x00525F6C> _numAnimations;
    static loco_global<TileAnimation[maxAnimations], 0x0094C6DC> _animations;

    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<Pos2[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;

    static TileElement* InvalidTile = reinterpret_cast<TileElement*>(static_cast<intptr_t>(-1));

    // 0x00461179
    void initialise()
    {
        call(0x00461179);
    }

    stdx::span<TileElement> getElements()
    {
        return stdx::span<TileElement>(_elements, getElementsEnd());
    }

    void setMapSelectionArea(const Pos2& locA, const Pos2& locB)
    {
        _mapSelectionAX = locA.x;
        _mapSelectionAY = locA.y;
        _mapSelectionBX = locB.x;
        _mapSelectionBY = locB.y;
    }

    std::pair<Pos2, Pos2> getMapSelectionArea()
    {
        return std::make_pair(Pos2{ _mapSelectionAX, _mapSelectionAY }, Pos2{ _mapSelectionBX, _mapSelectionBY });
    }

    void setMapSelectionCorner(const uint8_t corner)
    {
        _word_F2448E = corner;
    }

    uint8_t getMapSelectionCorner()
    {
        return _word_F2448E;
    }

    TileElement* getElementsEnd()
    {
        return _elementsEnd;
    }

    void setElements(stdx::span<TileElement> elements)
    {
        TileElement* dst = _elements;
        std::memset(dst, 0, maxElements * sizeof(TileElement));
        std::memcpy(dst, elements.data(), elements.size_bytes());
        TileManager::updateTilePointers();
    }

    TileElement** getElementIndex()
    {
        return _tiles.get();
    }

    Tile get(TilePos2 pos)
    {
        size_t index = (pos.y << 9) | pos.x;
        auto data = _tiles[index];
        if (data == InvalidTile)
        {
            data = nullptr;
        }
        return Tile(pos, data);
    }

    Tile get(Pos2 pos)
    {
        return get(pos.x, pos.y);
    }

    Tile get(coord_t x, coord_t y)
    {
        return get(TilePos2(x / Map::tile_size, y / Map::tile_size));
    }

    /**
     * Return the absolute height of an element, given its (x, y) coordinates
     * remember to & with 0xFFFF if you don't want water affecting results
     *
     * @param x @<ax>
     * @param y @<cx>
     * @return height @<edx>
     *
     * 0x00467297 rct2: 0x00662783 (numbers different)
     */
    TileHeight getHeight(const Pos2& pos)
    {
        TileHeight height{ 16, 0 };
        // Off the map
        if ((unsigned)pos.x >= (Map::map_width - 1) || (unsigned)pos.y >= (Map::map_height - 1))
            return height;

        auto tile = TileManager::get(pos);
        // Get the surface element for the tile
        auto surfaceEl = tile.surface();

        if (surfaceEl == nullptr)
        {
            return height;
        }

        height.waterHeight = surfaceEl->water() * 16;
        height.landHeight = surfaceEl->baseZ() * 4;

        const auto slope = surfaceEl->slopeCorners();
        if (slope == SurfaceSlope::flat)
        {
            // Flat surface requires no further calculations.
            return height;
        }

        int8_t quad = 0;
        int8_t quad_extra = 0; // which quadrant the element is in?
                               // quad_extra is for extra height tiles

        constexpr uint8_t TILE_SIZE = 31;

        // Subtile coords
        const auto xl = pos.x & 0x1f;
        const auto yl = pos.y & 0x1f;

        // Slope logic:
        // Each of the four bits in slope represents that corner being raised
        // slope == 15 (all four bits) is not used and slope == 0 is flat
        // If the extra_height bit is set, then the slope goes up two z-levels

        // We arbitrarily take the SW corner to be closest to the viewer

        // One corner up
        switch (slope)
        {
            case SurfaceSlope::n_corner_up:
                quad = xl + yl - TILE_SIZE;
                break;
            case SurfaceSlope::e_corner_up:
                quad = xl - yl;
                break;
            case SurfaceSlope::s_corner_up:
                quad = TILE_SIZE - yl - xl;
                break;
            case SurfaceSlope::w_corner_up:
                quad = yl - xl;
                break;
        }
        // If the element is in the quadrant with the slope, raise its height
        if (quad > 0)
        {
            height.landHeight += quad / 2;
        }

        // One side up
        switch (slope)
        {
            case SurfaceSlope::ne_side_up:
                height.landHeight += xl / 2 + 1;
                break;
            case SurfaceSlope::se_side_up:
                height.landHeight += (TILE_SIZE - yl) / 2;
                break;
            case SurfaceSlope::nw_side_up:
                height.landHeight += yl / 2;
                height.landHeight++;
                break;
            case SurfaceSlope::sw_side_up:
                height.landHeight += (TILE_SIZE - xl) / 2;
                break;
        }

        // One corner down
        switch (slope)
        {
            case SurfaceSlope::w_corner_dn:
                quad_extra = xl + TILE_SIZE - yl;
                quad = xl - yl;
                break;
            case SurfaceSlope::s_corner_dn:
                quad_extra = xl + yl;
                quad = xl + yl - TILE_SIZE - 1;
                break;
            case SurfaceSlope::e_corner_dn:
                quad_extra = TILE_SIZE - xl + yl;
                quad = yl - xl;
                break;
            case SurfaceSlope::n_corner_dn:
                quad_extra = (TILE_SIZE - xl) + (TILE_SIZE - yl);
                quad = TILE_SIZE - yl - xl - 1;
                break;
        }

        if (surfaceEl->isSlopeDoubleHeight())
        {
            height.landHeight += quad_extra / 2;
            height.landHeight++;
            return height;
        }

        // This tile is essentially at the next height level
        height.landHeight += 0x10;
        // so we move *down* the slope
        if (quad < 0)
        {
            height.landHeight += quad / 2;
        }

        // Valleys
        switch (slope)
        {
            case SurfaceSlope::w_e_valley:
                if (xl + yl <= TILE_SIZE + 1)
                {
                    return height;
                }
                quad = TILE_SIZE - xl - yl;
                break;
            case SurfaceSlope::n_s_valley:
                quad = xl - yl;
                break;
        }

        if (quad > 0)
        {
            height.landHeight += quad / 2;
        }

        return height;
    }

    static void clearTilePointers()
    {
        std::fill(_tiles.begin(), _tiles.end(), InvalidTile);
    }

    static void set(TilePos2 pos, TileElement* elements)
    {
        _tiles[(pos.y * map_pitch) + pos.x] = elements;
    }

    // 0x00461348
    void updateTilePointers()
    {
        clearTilePointers();

        TileElement* el = _elements;
        for (tile_coord_t y = 0; y < map_rows; y++)
        {
            for (tile_coord_t x = 0; x < map_columns; x++)
            {
                set(TilePos2(x, y), el);

                // Skip remaining elements on this tile
                do
                {
                    el++;
                } while (!(el - 1)->isLast());
            }
        }

        _elementsEnd = el;
    }

    // 0x0046148F
    void reorganise()
    {
        Ui::setCursor(Ui::CursorId::busy);

        try
        {
            // Allocate a temporary buffer and tighly pack all the tile elements in the map
            std::vector<TileElement> tempBuffer;
            tempBuffer.resize(maxElements * sizeof(TileElement));

            size_t numElements = 0;
            for (tile_coord_t y = 0; y < map_rows; y++)
            {
                for (tile_coord_t x = 0; x < map_columns; x++)
                {
                    auto tile = get(TilePos2(x, y));
                    for (const auto& element : tile)
                    {
                        tempBuffer[numElements] = element;
                        numElements++;
                    }
                }
            }

            // Copy organised elements back to original element buffer
            std::memcpy(_elements, tempBuffer.data(), numElements * sizeof(TileElement));

            // Zero all unused elements
            auto remainingElements = maxElements - numElements;
            std::memset(_elements + numElements, 0, remainingElements * sizeof(TileElement));

            updateTilePointers();

            // Note: original implementation did not revert the cursor
            Ui::setCursor(Ui::CursorId::pointer);
        }
        catch (const std::bad_alloc&)
        {
            exitWithError(4370, StringIds::null);
            return;
        }
    }

    // 0x0045F1A7
    Pos2 screenGetMapXY(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        call(0x0045F1A7, regs);
        Pos2 pos = { regs.ax, regs.bx };
        return pos;
    }

    uint16_t setMapSelectionTiles(int16_t x, int16_t y)
    {
        auto pos = screenGetMapXY(x, y);

        uint16_t xPos = pos.x;
        uint16_t yPos = pos.y;
        uint8_t count = 0;

        if (xPos == 0x8000)
            return 0x8000;

        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
        {
            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            count++;
        }

        if (_word_F2448E != 4)
        {
            _word_F2448E = 4;
            count++;
        }

        uint16_t toolSizeA = _adjustToolSize;

        if (!toolSizeA)
            toolSizeA = 1;

        toolSizeA = toolSizeA << 5;
        uint16_t toolSizeB = toolSizeA;
        toolSizeB -= 32;
        toolSizeA = toolSizeA >> 1;
        toolSizeA -= 16;
        xPos -= toolSizeA;
        yPos -= toolSizeA;
        xPos &= 0xFFE0;
        yPos &= 0xFFE0;

        if (xPos != _mapSelectionAX)
        {
            _mapSelectionAX = xPos;
            count++;
        }

        if (yPos != _mapSelectionAY)
        {
            _mapSelectionAY = yPos;
            count++;
        }

        xPos += toolSizeB;
        yPos += toolSizeB;

        if (xPos != _mapSelectionBX)
        {
            _mapSelectionBX = xPos;
            count++;
        }

        if (yPos != _mapSelectionBY)
        {
            _mapSelectionBY = yPos;
            count++;
        }

        mapInvalidateSelectionRect();

        return count;
    }

    // 0x0045FD8E
    Pos3 screenPosToMapPos(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        call(0x0045FD8E, regs);
        Pos3 pos = { regs.ax, regs.bx, regs.cx };
        return pos;
    }

    uint16_t setMapSelectionSingleTile(int16_t x, int16_t y, bool setQuadrant)
    {
        auto pos = screenPosToMapPos(x, y);

        uint16_t xPos = pos.x;
        uint16_t yPos = pos.y;
        uint16_t cursorQuadrant = pos.z;

        if (xPos == 0x8000)
            return 0x8000;

        auto count = 0;
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
        {
            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            count++;
        }

        if (setQuadrant && _word_F2448E != cursorQuadrant)
        {
            _word_F2448E = cursorQuadrant;
            count++;
        }
        else if (!setQuadrant && _word_F2448E != 4)
        {
            _word_F2448E = 4;
            count++;
        }

        if (xPos != _mapSelectionAX)
        {
            _mapSelectionAX = xPos;
            count++;
        }

        if (yPos != _mapSelectionAY)
        {
            _mapSelectionAY = yPos;
            count++;
        }

        if (xPos != _mapSelectionBX)
        {
            _mapSelectionBX = xPos;
            count++;
        }

        if (yPos != _mapSelectionBY)
        {
            _mapSelectionBY = yPos;
            count++;
        }

        mapInvalidateSelectionRect();

        return count;
    }

    // 0x004610F2
    void mapInvalidateSelectionRect()
    {
        if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
        {
            for (coord_t x = _mapSelectionAX; x <= _mapSelectionBX; x += 32)
            {
                for (coord_t y = _mapSelectionAY; y <= _mapSelectionBY; y += 32)
                {
                    mapInvalidateTileFull({ x, y });
                }
            }
        }
    }

    // 0x004CBE5F
    // regs.ax: pos.x
    // regs.cx: pos.y
    void mapInvalidateTileFull(Map::Pos2 pos)
    {
        Ui::ViewportManager::invalidate(pos, 0, 1120, ZoomLevel::eighth);
    }

    // 0x0046112C
    void mapInvalidateMapSelectionTiles()
    {
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enableConstruct))
            return;

        for (uint16_t index = 0; index < mapSelectedTilesSize; ++index)
        {
            auto& position = _mapSelectedTiles[index];
            if (position.x == -1)
                break;
            mapInvalidateTileFull(position);
        }
    }

    // 0x0046A747
    void resetSurfaceClearance()
    {
        for (coord_t y = 0; y < map_height; y += tile_size)
        {
            for (coord_t x = 0; x < map_width; x += tile_size)
            {
                auto tile = get(x, y);
                auto surface = tile.surface();
                if (surface != nullptr && surface->slope() == 0)
                {
                    surface->setClearZ(surface->baseZ());
                }
            }
        }
    }

    // 0x004612A6
    void createAnimation(uint8_t type, const Pos2& pos, tile_coord_t baseZ)
    {
        if (_numAnimations >= maxAnimations)
            return;

        for (size_t i = 0; i < _numAnimations; i++)
        {
            auto& animation = _animations[i];
            if (animation.type == type && animation.pos == pos && animation.baseZ == baseZ)
            {
                return;
            }
        }

        auto& newAnimation = _animations[_numAnimations++];
        newAnimation.baseZ = baseZ;
        newAnimation.type = type;
        newAnimation.pos = pos;
    }

    // 0x00461166
    void resetAnimations()
    {
        _numAnimations = 0;
    }

    // 0x004C5596
    uint16_t countSurroundingWaterTiles(const Pos2& pos)
    {
        // Search a 10x10 area centred at pos.
        // Initial tile position is the top left of the area.
        auto initialTilePos = Map::TilePos2(pos) - Map::TilePos2(5, 5);

        uint16_t surroundingWaterTiles = 0;
        for (uint8_t yOffset = 0; yOffset < 11; yOffset++)
        {
            for (uint8_t xOffset = 0; xOffset < 11; xOffset++)
            {
                auto tilePos = initialTilePos + Map::TilePos2(xOffset, yOffset);
                if (!Map::validCoords(tilePos))
                    continue;

                auto tile = get(tilePos);
                auto* surface = tile.surface();
                if (surface != nullptr && surface->water() > 0)
                    surroundingWaterTiles++;
            }
        }

        return surroundingWaterTiles;
    }

    // 0x004BE048
    uint16_t countSurroundingTrees(const Pos2& pos)
    {
        // Search a 10x10 area centred at pos.
        // Initial tile position is the top left of the area.
        auto initialTilePos = Map::TilePos2(pos) - Map::TilePos2(5, 5);

        uint16_t surroundingTrees = 0;
        for (uint8_t yOffset = 0; yOffset < 11; yOffset++)
        {
            for (uint8_t xOffset = 0; xOffset < 11; xOffset++)
            {
                auto tilePos = initialTilePos + Map::TilePos2(xOffset, yOffset);
                if (!Map::validCoords(tilePos))
                    continue;

                auto tile = get(tilePos);
                for (auto& element : tile)
                {
                    // NB: vanilla was checking for trees above the surface element.
                    // This has been omitted from our implementation.
                    auto* tree = element.asTree();
                    if (tree == nullptr)
                        continue;

                    if (tree->isGhost())
                        continue;

                    surroundingTrees++;
                }
            }
        }

        return surroundingTrees;
    }

    void registerHooks()
    {
        registerHook(
            0x004612A6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                createAnimation(regs.dh, { regs.ax, regs.cx }, regs.dl);
                return 0;
            });

        // This hook can be removed once sub_4599B3 has been implemented
        registerHook(
            0x004BE048,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                regs.dx = countSurroundingTrees({ regs.ax, regs.cx });
                return 0;
            });

        registerHook(
            0x004C5596,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                regs.dx = countSurroundingWaterTiles({ regs.ax, regs.cx });
                return 0;
            });
    }
}
