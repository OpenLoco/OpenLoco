#include "Input.h"
#include <fmt/format.h>

namespace OpenLoco::Input
{
    std::string_view format_as(MouseButton state)
    {
        std::string_view name = "Input::mouseButton::<invalid>";
        switch (state)
        {
            case OpenLoco::Input::MouseButton::released:
                name = "Input::MouseButton::released";
                break;
            case OpenLoco::Input::MouseButton::leftPressed:
                name = "Input::MouseButton::leftPressed";
                break;
            case OpenLoco::Input::MouseButton::leftReleased:
                name = "Input::MouseButton::leftReleased";
                break;
            case OpenLoco::Input::MouseButton::rightPressed:
                name = "Input::MouseButton::rightPressed";
                break;
            case OpenLoco::Input::MouseButton::rightReleased:
                name = "Input::MouseButton::rightReleased";
                break;
            default:
                break;
        }
        return name;
    }

    std::string_view format_as(State value)
    {
        std::string_view name = "Input::state::<invalid>";
        switch (value)
        {
            case OpenLoco::Input::State::reset:
                name = "Input::State::reset";
                break;
            case OpenLoco::Input::State::normal:
                name = "Input::State::normal";
                break;
            case OpenLoco::Input::State::widgetPressed:
                name = "Input::State::widgetPressed";
                break;
            case OpenLoco::Input::State::positioningWindow:
                name = "Input::State::positioningWindow";
                break;
            case OpenLoco::Input::State::viewportRight:
                name = "Input::State::viewportRight";
                break;
            case OpenLoco::Input::State::dropdownActive:
                name = "Input::State::dropdownActive";
                break;
            case OpenLoco::Input::State::viewportLeft:
                name = "Input::State::viewportLeft";
                break;
            case OpenLoco::Input::State::scrollLeft:
                name = "Input::State::scrollLeft";
                break;
            case OpenLoco::Input::State::resizing:
                name = "Input::State::resizing";
                break;
            case OpenLoco::Input::State::scrollRight:
                name = "Input::State::scrollRight";
                break;
            default:
                break;
        }
        return name;
    }
}
