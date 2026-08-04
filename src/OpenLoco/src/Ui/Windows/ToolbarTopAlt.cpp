#include "Audio/Audio.h"
#include "Config.h"
#include "EditorController.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "Scenario/ScenarioOptions.h"
#include "Ui/Dropdown.h"
#include "Ui/Screenshot.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ImageButtonAltWidget.h"
#include "Ui/WindowManager.h"
#include "Ui/Windows/ToolbarTopCommon.h"
#include "Vehicles/Vehicle.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

namespace OpenLoco::Ui::Windows::ToolbarTop::Editor
{
    void open()
    {
        ToolbarTop::Game::open();
    }
}
