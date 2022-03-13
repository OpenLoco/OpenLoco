#pragma once

#include "Tile.h"

namespace OpenLoco::Map
{
#pragma pack(push, 1)
    struct TileLoop
    {
    private:
        Pos2 _pos;

    public:
        Pos2 current() const { return _pos; }
        Pos2 next()
        {
            _pos.x += tile_size;
            if (_pos.x >= map_width - 1)
            {
                _pos.x = 0;
                _pos.y += tile_size;
                if (_pos.y >= map_height - 1)
                {
                    _pos.y = 0;
                }
            }
            return _pos;
        }
    };
#pragma pack(pop)

    // Loops over a range from bottomLeft to topRight inclusive
    struct TilePosRangeView
    {
    private:
        TilePos2 _bottomLeft;
        TilePos2 _topRight;

        class Iterator
        {
        private:
            const TilePos2& _bottomLeft;
            const TilePos2& _topRight;
            TilePos2 _pos;

        public:
            Iterator(const TilePos2& bottomLeft, const TilePos2& topRight)
                : _bottomLeft(bottomLeft)
                , _topRight(topRight)
                , _pos(bottomLeft)
            {
            }

            Iterator& operator++()
            {
                if (_pos.x >= _topRight.x)
                {
                    _pos.x = _bottomLeft.x;
                    _pos.y++;
                }
                else
                {
                    _pos.x++;
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator retval = *this;
                ++(*this);
                return retval;
            }

            bool operator==(const Iterator& other) const
            {
                return _pos == other._pos;
            }

            bool operator!=(const Iterator& other) const
            {
                return !(*this == other);
            }

            const TilePos2& operator*()
            {
                return _pos;
            }
            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = const TilePos2;
            using pointer = TilePos2*;
            using reference = const TilePos2&;
            using iterator_category = std::forward_iterator_tag;
        };

    public:
        TilePosRangeView(const TilePos2& bottomLeft, const TilePos2& topRight)
            : _bottomLeft(bottomLeft)
            , _topRight(topRight)
        {
        }

        Iterator begin() { return Iterator(_bottomLeft, _topRight); }
        Iterator end()
        {
            // End iterator must be 1 step past the end so that loop is inclusive
            return Iterator(TilePos2(_bottomLeft.x, _topRight.y + 1), _topRight);
        }
    };
}
