#pragma once

namespace OpenLoco::World
{
    struct IndustryElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintIndustry(PaintSession& session, const World::IndustryElement& elIndustry);
}
