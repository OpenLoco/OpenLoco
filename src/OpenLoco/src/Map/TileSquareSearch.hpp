#include <OpenLoco/Engine/World.hpp>
#include <cassert>

namespace OpenLoco::World
{
    template<size_t searchSize>
    constexpr auto kSquareSearchRange = []() {
        /* Searches in a square of increasing size
         * X, Y, Z, J, K
         *
         *     -4-3-2-1 0 1 2 3 4 5
         *     ____________________
         *  4 | K K K K K K K K K K
         *  3 | K J J J J J J J J K
         *  2 | K J Z Z Z Z Z Z J K
         *  1 | K J Z Y Y Y Y Z J K
         *  0 | K J Z Y X X Y Z J K
         * -1 | K J Z Y X X Y Z J K
         * -2 | K J Z Y Y Y Y Z J K
         * -3 | K J Z Z Z Z Z Z J K
         * -4 | K J J J J J J J J K
         * -5 | K K K K K K K K K K
         */
        static_assert((searchSize % 2) == 1, "Must not be an even value");
        std::array<World::TilePos2, (searchSize + 1) * (searchSize + 1)> range{};
        // 0x00503C6C
        std::array<World::TilePos2, 4> kDirections = {
            World::TilePos2{ -1, 0 },
            World::TilePos2{ 0, 1 },
            World::TilePos2{ 1, 0 },
            World::TilePos2{ 0, -1 },
        };

        World::TilePos2 pos{ 0, 0 };
        uint8_t k = 0;
        for (uint8_t i = 1; i <= searchSize; i += 2)
        {
            for (uint8_t direction = 0; direction < 4; ++direction)
            {
                for (auto j = i; j != 0; --j)
                {
                    range[k++] = pos;
                    pos += kDirections[direction];
                }
            }
            pos += World::TilePos2{ 1, -1 };
        }
        return range;
    }();

    // 0x00463BD2
    template<typename Func>
    static void squareSearch(const World::Pos2& centre, [[maybe_unused]] uint8_t searchSize, Func&& predicate)
    {
        assert(searchSize == 9);
        for (auto& offset : kSquareSearchRange<9>)
        {
            const World::Pos2 pos = World::toWorldSpace(offset) + centre;
            if (World::validCoords(pos))
            {
                if (!predicate(pos))
                {
                    return;
                }
            }
        }
    }
}
