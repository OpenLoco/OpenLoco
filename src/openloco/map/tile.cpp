#include "tile.h"
#include "../industrymgr.h"
#include "../objects/objectmgr.h"
#include <cassert>

using namespace openloco;
using namespace openloco::map;

const uint8_t* tile_element_base::data() const
{
    return (uint8_t*)this;
}

element_type tile_element_base::type() const
{
    return (element_type)((_type & 0x3C) >> 2);
}

bool tile_element_base::is_last() const
{
    return (_flags & element_flags::last) != 0;
}

building_object* building_element::object() const
{
    return objectmgr::get<building_object>(object_id());
}

industry* industry_element::industry() const
{
    return industrymgr::get(industry_id());
}

tile::tile(tile_coord_t x, tile_coord_t y, tile_element* data)
    : _data(data)
    , x(x)
    , y(y)
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

size_t tile::index_of(const tile_element_base* element) const
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
