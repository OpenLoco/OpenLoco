#pragma once

namespace OpenLoco::S5
{
    struct Options;
}

namespace OpenLoco::Map::MapGenerator
{
    void generate(const S5::Options& options);
}
