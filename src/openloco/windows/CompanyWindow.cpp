#include "../company.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyWindow
{
    namespace common
    {

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                                 \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                            \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),      \
        make_widget({ 0, 41 }, { frameWidth, 120 }, widget_type::panel, 1),                                                                    \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_owner_and_status),          \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_headquarters_and_details), \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_colour_scheme),            \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_finances),                 \
        make_remap_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_cargo_delivered),                 \
        make_remap_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_challenge_for_this_game), \
        make_widget({ 0, 14 }, { 26, 26 }, widget_type::wt_9, 0, image_ids::null, string_ids::tooltip_select_company)
    }

    namespace status
    {
        [[maybe_unused]] static widget_t _widgets[] = {
            commonWidgets(270, 182, string_ids::title_company),
            make_widget({ 3, 160 }, { 242, 21 }, widget_type::wt_13, 1),
            make_widget({ 3, 44 }, { 96, 120 }, widget_type::viewport, 1, -2),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 178, 57 }, { 66, 66 }, widget_type::wt_9, 1, image_ids::null),
            make_widget({ 154, 124 }, { 112, 22 }, widget_type::wt_9, 1, image_ids::null, string_ids::tooltip_change_owner_name),
            widget_end(),
        };
    }

    // 0x0043454F
    void open(company_id_t companyId)
    {
        registers regs;
        regs.eax = companyId;
        call(0x0043454F, regs);
    }

    namespace details
    {
        [[maybe_unused]] static widget_t _widgets[] = {
            commonWidgets(340, 194, string_ids::title_company_details),
            make_widget({ 219, 54 }, { 96, 120 }, widget_type::viewport, 1, -2),
            make_widget({ 315, 92 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::tooltip_build_or_move_headquarters),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            widget_end(),
        };
    }

    namespace colour_scheme
    {

        [[maybe_unused]] static widget_t _widgets[] = {
            commonWidgets(265, 252, string_ids::title_company_colour_scheme),
            make_widget({ 15, 81 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_steam_locomotives, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 98 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_diesel_locomotives, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 115 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_electric_locomotives, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 132 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_multiple_units, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 149 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_passenger_vehicles, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 166 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_freight_vehicles, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 183 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_buses, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 200 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_trucks, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 217 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_aircraft, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 234 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_ships, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 221, 48 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 78 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 95 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 112 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 129 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 146 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 163 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 180 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 197 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 214 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 231 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 239, 48 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 78 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 95 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 112 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 129 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 146 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 163 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 180 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 197 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 214 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 231 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            widget_end(),
        };
    }

    namespace finances
    {
        [[maybe_unused]] static widget_t _widgets[] = {
            commonWidgets(636, 319, string_ids::title_company_finances),
            make_widget({ 133, 45 }, { 499, 215 }, widget_type::scrollview, 1, horizontal),
            make_stepper_widgets({ 87, 264 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::company_current_loan_value),
            widget_end(),
        };
    }

    void openFinances(company_id_t companyId)
    {
        registers regs;
        regs.eax = companyId;
        call(0x004345EE, regs);
    }

    namespace cargo_delivered
    {
        [[maybe_unused]] static widget_t _widgets[] = {
            commonWidgets(340, 382, string_ids::title_company_cargo_delivered),
            widget_end(),
        };
    }

    namespace challenge
    {
        [[maybe_unused]] static widget_t _widgets[] = {
            commonWidgets(320, 182, string_ids::title_company_challenge),
            widget_end(),
        };
    }
}
