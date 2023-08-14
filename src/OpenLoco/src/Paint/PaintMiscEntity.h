#pragma once

#include "Effects/Effect.h"
#include "Paint.h"

namespace OpenLoco::Paint
{

    /*
     * @param base->x @<ax>
     * @param base->y @<cx>
     * @param base->z @<dx>
     * @param ((base->sprite_yaw + (session.getRotation() << 4)) & 0x3F) @<ebx>
     * @param base @<esi>
     */
    void paintMiscEntity(PaintSession& session, MiscBase* base);
}
