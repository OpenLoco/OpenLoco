#include "tile.h"
#include <cassert>

using namespace openloco::map;

const uint8_t* tile_element::data() const
{
    return (uint8_t*)this;
}

element_type tile_element::type() const
{
    return (element_type)(_type & 0x3C);
}

bool tile_element::is_last() const
{
    return (_flags & 0x80) != 0;
}

tile::tile(tile_coord_t x, tile_coord_t y, tile_element* data)
    : x(x)
    , y(y)
    , _data(data)
{
}

bool tile::is_null() const
{
    return _data == nullptr;
}

tile_element* tile::begin()
{
    return _data;
}

tile_element* tile::begin() const
{
    return const_cast<tile&>(*this).begin();
}

tile_element* tile::end()
{
    auto el = _data;
    do
    {
        el++;
    } while (!(el - 1)->is_last());
    return el;
}

tile_element* tile::end() const
{
    return const_cast<tile&>(*this).end();
}

size_t tile::size()
{
    return end() - begin();
}

tile_element* tile::operator[](size_t i)
{
#if DEBUG
    assert(i < size());
#endif
    return &_data[i];
}

size_t tile::index_of(const tile_element* element) const
{
    size_t i = 0;
    for (const auto& tile : *this)
    {
        if (&tile == element)
        {
            return i;
        }
        i++;
    }
    return npos;
}

surface_element* tile::surface()
{
    surface_element* result = nullptr;
    for (auto& tile : *this)
    {
        result = tile.as_surface();
        if (result != nullptr)
        {
            break;
        }
    }
    return result;
}
