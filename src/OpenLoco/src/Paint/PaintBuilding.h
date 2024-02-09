#pragma once

namespace OpenLoco::World
{
    struct BuildingElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintBuilding(PaintSession& session, const World::BuildingElement& elBuilding);
}
