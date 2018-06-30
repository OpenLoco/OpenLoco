#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::options
{
    static widget_t _widgets_display[] = {
        make_widget({ 0, 0 }, { 366, 144 }, widget_type::frame, 0, -1),
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, string_ids::options_title_display),
        make_widget({ 351, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 366, 102 }, widget_type::panel, 1, -1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options),
        make_widget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0066),
        make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 10, 65 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_0663, string_ids::STR_0664),
        make_widget({ 10, 80 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_0665, string_ids::STR_0666),
        make_widget({ 183, 94 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::null, string_ids::STR_1099),
        make_widget({ 344, 95 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::STR_1099),
        make_widget({ 183, 109 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::null, string_ids::STR_1100),
        make_widget({ 344, 110 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::STR_1100),
        make_widget({ 183, 124 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::null),
        make_widget({ 344, 125 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
    };

    static widget_t _widgets_sound[] = {
        make_widget({ 0, 0 }, { 366, 97 }, widget_type::frame, 0, -1),
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, string_ids::options_title_sound),
        make_widget({ 351, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 366, 102 }, widget_type::panel, 1, -1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options),
        make_widget({ 10, 49 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::STR_0085),
        make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 183, 64 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0090),
        make_widget({ 344, 65 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 10, 79 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1010, string_ids::STR_1011),
    };

    static widget_t _widgets_music[] = {
        make_widget({ 0, 0 }, { 366, 129 }, widget_type::frame, 0, -1),
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, string_ids::options_title_music),
        make_widget({ 351, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 366, 102 }, widget_type::panel, 1, -1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options),
        make_widget({ 160, 49 }, { 196, 12 }, widget_type::wt_18, 1, string_ids::STR_0085),
        make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 10, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_stop, string_ids::STR_1536),
        make_widget({ 34, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_play, string_ids::STR_1537),
        make_widget({ 58, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_next, string_ids::STR_1538),
        make_widget({ 256, 64 }, { 109, 24 }, widget_type::wt_5, 1, -1, string_ids::STR_1548),
        make_widget({ 10, 93 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::STR_0086),
        make_widget({ 344, 94 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 183, 108 }, { 173, 12 }, widget_type::wt_11, 1, string_ids::STR_1542, string_ids::STR_1543),
    };

    static widget_t _widgets_regional[] = {
        make_widget({ 0, 0 }, { 366, 147 }, widget_type::frame, 0, -1),
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, string_ids::options_title_regional),
        make_widget({ 351, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 366, 102 }, widget_type::panel, 1, -1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options),
        make_widget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0091),
        make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 183, 64 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0088),
        make_widget({ 344, 65 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
        make_widget({ 183, 84 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0090, string_ids::STR_1502),
        make_widget({ 344, 85 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::STR_1502),
        make_widget({ 183, 99 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_1506, string_ids::STR_1503),
        make_widget({ 344, 100 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::STR_1503),
        make_widget({ 10, 114 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1500, string_ids::STR_1501),
        make_widget({ 10, 129 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1498, string_ids::STR_1499),
    };

    static widget_t _widgets_controls[] = {
        make_widget({ 0, 0 }, { 366, 87 }, widget_type::frame, 0, -1),
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, string_ids::options_title_controls),
        make_widget({ 351, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 366, 102 }, widget_type::panel, 1, -1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options),
        make_widget({ 10, 50 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1002, string_ids::STR_1003),
        make_widget({ 26, 64 }, { 160, 12 }, widget_type::wt_11, 1, string_ids::STR_0701, string_ids::STR_1004),
    };

    static widget_t _widgets_misc[] = {
        make_widget({ 0, 0 }, { 420, 102 }, widget_type::frame, 0, -1),
        make_widget({ 1, 1 }, { 418, 13 }, widget_type::caption_25, 0, string_ids::options_title_miscellaneous),
        make_widget({ 405, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 366, 102 }, widget_type::panel, 1, -1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options),
        make_widget({ 10, 49 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::STR_1919, string_ids::STR_1920),
        make_widget({ 335, 64 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::STR_1700),
        make_widget({ 10, 79 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::STR_2089, string_ids::STR_2090),
    };

    void open()
    {
        call(0x004BF7B9);
    }
}
