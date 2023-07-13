#include "Debug.h"
#include <imgui.h>

namespace OpenLoco::Ui::Windows::Debug
{
    static bool _open = true;

    void open()
    {
        _open = true;
    }

    void draw()
    {
        if (!_open)
            return;

        ImGui::ShowDemoWindow(&_open);

        if (ImGui::Begin("Debug", &_open))
        {
            ImGui::Text("Hello, world!");
        }
        ImGui::End();
    }

}
