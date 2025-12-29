#include "Scenario/ScenarioPreview.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Map/TileManager.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Scenario
{
    void drawSavePreviewImage(void* pixels, Ui::Size size)
    {
        auto mainViewport = Ui::WindowManager::getMainViewport();
        if (mainViewport == nullptr)
        {
            return;
        }

        const auto mapPosXY = mainViewport->getCentreMapPosition();
        const auto mapPosXYZ = World::Pos3(mapPosXY.x, mapPosXY.y, coord_t{ World::TileManager::getHeight(mapPosXY) });

        Ui::Viewport saveVp{};
        saveVp.x = 0;
        saveVp.y = 0;
        saveVp.width = size.width;
        saveVp.height = size.height;
        saveVp.flags = Ui::ViewportFlags::hideTownNames | Ui::ViewportFlags::hideStationNames;
        saveVp.zoom = ZoomLevel::half;
        saveVp.viewWidth = size.width << saveVp.zoom;
        saveVp.viewHeight = size.height << saveVp.zoom;

        const auto viewPos = saveVp.centre2dCoordinates(mapPosXYZ);
        saveVp.viewX = viewPos.x;
        saveVp.viewY = viewPos.y;

        Gfx::RenderTarget rt{};
        rt.bits = static_cast<uint8_t*>(pixels);
        rt.x = 0;
        rt.y = 0;
        rt.width = size.width;
        rt.height = size.height;
        rt.pitch = 0;
        rt.zoomLevel = saveVp.zoom;

        auto& drawingEngine = Gfx::getDrawingEngine();
        auto& drawingCtx = drawingEngine.getDrawingContext();

        drawingCtx.pushRenderTarget(rt);
        saveVp.render(drawingCtx);
        drawingCtx.popRenderTarget();
    }
}
