#include "PaintEntity.h"
#include "../Config.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Misc.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Vehicles/Vehicle.h"
#include "Paint.h"
#include "PaintMiscEntity.h"
#include "PaintVehicle.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    template<typename FilterType>
    static void paintEntitiesWithFilter(PaintSession& session, const Map::Pos2& loc, FilterType&& filter)
    {
        auto* rt = session.getRenderTarget();
        if (Config::get().vehiclesMinScale < rt->zoom_level)
        {
            return;
        }

        if (loc.x >= 0x4000 || loc.y >= 0x4000)
        {
            return;
        }

        EntityManager::EntityTileList entities(loc);
        for (auto* entity : entities)
        {
            // TODO: Create a rect from context dims
            auto left = rt->x;
            auto top = rt->y;
            auto right = left + rt->width;
            auto bottom = top + rt->height;

            // TODO: Create a rect from sprite dims and use a contains function
            if (entity->spriteTop > bottom)
            {
                continue;
            }
            if (entity->spriteBottom <= top)
            {
                continue;
            }
            if (entity->spriteLeft > right)
            {
                continue;
            }
            if (entity->spriteRight <= left)
            {
                continue;
            }
            if (!filter(entity))
            {
                continue;
            }
            session.setCurrentItem(entity);
            session.setEntityPosition(entity->position);
            session.setItemType(InteractionItem::entity);
            switch (entity->base_type)
            {
                case EntityBaseType::vehicle:
                    paintVehicleEntity(session, entity->asBase<Vehicles::VehicleBase>());
                    break;
                case EntityBaseType::misc:
                    paintMiscEntity(session, entity->asBase<MiscBase>());
                    break;
                case EntityBaseType::null:
                    // Nothing to paint
                    break;
            }
        }
    }

    // 0x0046FA88
    void paintEntities(PaintSession& session, const Map::Pos2& loc)
    {
        paintEntitiesWithFilter(session, loc, [](const EntityBase*) { return true; });
    }

    static bool isEntityFlyingOrFloating(const EntityBase* entity)
    {
        auto* vehicle = entity->asBase<Vehicles::VehicleBase>();
        if (vehicle == nullptr)
        {
            return false;
        }
        switch (vehicle->getTransportMode())
        {
            case TransportMode::air:
            case TransportMode::water:
                return true;
            case TransportMode::rail:
            case TransportMode::road:
                return false;
        }
        return false;
    }

    // 0x0046FB67
    void paintEntities2(PaintSession& session, const Map::Pos2& loc)
    {
        paintEntitiesWithFilter(session, loc, isEntityFlyingOrFloating);
    }
}
