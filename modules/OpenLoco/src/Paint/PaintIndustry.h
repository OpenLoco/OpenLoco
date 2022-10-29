#pragma once

namespace OpenLoco::Map
{
    struct IndustryElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintIndustry(PaintSession& session, const Map::IndustryElement& elIndustry);
}
