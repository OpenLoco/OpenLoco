#include "../Audio/Audio.h"
#include "../Config.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/ObjectManager.h"
#include "../Ptr.h"
#include "../Scenario.h"
#include "../ScenarioManager.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ScenarioSelect
{
    constexpr Gfx::ui_size_t windowSize = { 610, 412 };

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            tab0,
            tab1,
            tab2,
            tab3,
            tab4,
            list,
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 610, 412 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 608, 13 }, widget_type::caption_25, 0, StringIds::select_scenario_for_new_game),
        makeWidget({ 595, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 48 }, { 610, 364 }, widget_type::wt_3, 1),
        makeRemapWidget({ 3, 15 }, { 91, 34 }, widget_type::wt_8, 1, ImageIds::wide_tab),
        makeRemapWidget({ 94, 15 }, { 91, 34 }, widget_type::wt_8, 1, ImageIds::wide_tab),
        makeRemapWidget({ 185, 15 }, { 91, 34 }, widget_type::wt_8, 1, ImageIds::wide_tab),
        makeRemapWidget({ 276, 15 }, { 91, 34 }, widget_type::wt_8, 1, ImageIds::wide_tab),
        makeRemapWidget({ 367, 15 }, { 91, 34 }, widget_type::wt_8, 1, ImageIds::wide_tab),
        makeWidget({ 3, 52 }, { 431, 356 }, widget_type::scrollview, 1, scrollbars::vertical),
        widgetEnd(),
    };

    constexpr auto rowHeight = 24;

    static window_event_list _events;

    static void initEvents();

    // 0x00443807
    static void initTabs(window* self)
    {
        uint16_t xPos = 3;
        for (int i = 0; i < 5; i++)
        {
            widget_t& widget = self->widgets[widx::tab0 + i];
            if (ScenarioManager::hasScenariosForCategory(i))
            {
                widget.type = widget_type::wt_8;
                widget.left = xPos;
                widget.right = xPos + 90;
                xPos += 91;
            }
            else
                widget.type = widget_type::none;
        }
    }

    // 0x00443946
    static void initList(window* self)
    {
        if (self->info == 0xFFFFFFFF)
            return;

        using namespace ScenarioManager;
        auto scenarioInfo = reinterpret_cast<ScenarioIndexEntry*>(self->info);
        if (hasScenarioInCategory(self->current_tab, scenarioInfo))
            return;

        // Reset currently selected scenario if it is not in the current category.
        self->info = 0xFFFFFFFF;
        self->invalidate();
    }

    // 0x00443868
    window* open()
    {
        window* self = WindowManager::bringToFront(WindowType::scenarioSelect);
        if (self != nullptr)
            return self;

        initEvents();

        self = WindowManager::createWindow(
            WindowType::scenarioSelect,
            Gfx::point_t({ static_cast<int16_t>(width() / 2 - windowSize.width / 2),
                           std::max<int16_t>(height() / 2 - windowSize.height / 2, 28) }),
            windowSize,
            WindowFlags::stick_to_front | WindowFlags::flag_12,
            &_events);

        self->widgets = _widgets;
        self->enabled_widgets = (1 << widx::close) | (1 << widx::tab0) | (1 << widx::tab1) | (1 << widx::tab2) | (1 << widx::tab3) | (1 << widx::tab4);
        self->initScrollWidgets();

        self->colours[0] = Colour::black;
        self->colours[1] = Colour::saturated_green;

        self->var_846 = 0xFFFF;
        self->var_85A = static_cast<int32_t>(0xFFFFFFFF);

        initTabs(self);

        // Select the last tab used, or the first available one.
        uint8_t selectedTab = Config::get().scenario_selected_tab;
        if (self->widgets[widx::tab0 + selectedTab].type == widget_type::none)
        {
            selectedTab = 0;
            for (int i = 0; i < 5; i++)
            {
                if (self->widgets[widx::tab0 + i].type == widget_type::none)
                {
                    selectedTab = i;
                    break;
                }
            }
        }

        self->current_tab = selectedTab;

        initList(self);

        return self;
    }

    // 0x00443995
    static void prepareDraw(window* self)
    {
        self->activated_widgets &= ~((1 << widx::tab0) | (1 << widx::tab1) | (1 << widx::tab2) | (1 << widx::tab3) | (1 << widx::tab4));
        self->activated_widgets |= (1ULL << (self->current_tab + static_cast<uint8_t>(widx::tab0)));
    }

    // 0x004439AF
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        Gfx::drawRectInset(dpi, self->x, self->y + 20, self->width, 41, self->colours[0], 0);

        // Draw widgets.
        self->draw(dpi);

        static const string_id scenarioGroupIds[] = {
            StringIds::scenario_group_beginner,
            StringIds::scenario_group_easy,
            StringIds::scenario_group_medium,
            StringIds::scenario_group_challenging,
            StringIds::scenario_group_expert,
        };

        // Draw tab captions.
        for (int i = 0; i < 5; i++)
        {
            widget_t& widget = self->widgets[widx::tab0 + i];
            if (widget.type == widget_type::none)
                continue;

            const auto offset = self->current_tab == i ? 1 : 0;
            auto origin = Gfx::point_t(widget.mid_x() + self->x, widget.mid_y() + self->y - 3 - offset);
            const string_id caption = scenarioGroupIds[i];

            Gfx::drawStringCentredWrapped(dpi, &origin, widget.width() - 4, Colour::black, StringIds::wcolour2_stringid, &caption);
        }

        // Scenario selected?
        if (self->info == 0 || self->info == 0xFFFFFFFF)
            return;

        using namespace ScenarioManager;
        auto scenarioInfo = reinterpret_cast<ScenarioIndexEntry*>(self->info);

        // Check if current currency object needs to be changed.
        auto isLoaded = ObjectManager::findIndex(scenarioInfo->currency);
        if (!isLoaded)
        {
            // Unload current object
            registers regs;
            regs.ebp = 0x0011264A4; // currencyMeta
            call(0x00471FF8, regs); // unload object

            // Load required object
            registers regs2;
            regs2.ebp = ToInt(&scenarioInfo->currency);
            call(0x00471BCE, regs2);
            call(0x0047237D); // reset_loaded_objects
            call(0x0046E07B); // load currency gfx
        }

        const int16_t baseX = self->x + self->widgets[widx::list].right + 4;
        const int16_t baseY = self->y + self->widgets[widx::panel].top + 5;
        const int16_t colWidth = self->widgets[widx::panel].right - self->widgets[widx::list].right - 6;

        int16_t x = baseX, y = baseY;

        // Scenario name
        {
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            strncpy(str, scenarioInfo->scenarioName, std::size(scenarioInfo->scenarioName));

            auto args = FormatArguments();
            args.push(StringIds::buffer_2039);

            x += colWidth / 2;
            Gfx::drawStringCentredClipped(*dpi, x, y, 170, Colour::black, StringIds::wcolour2_stringid, &args);

            y += 14;
        }

        // Outline for preview image
        {
            x = baseX + 20;
            Gfx::drawRectInset(dpi, x, y, 130, 130, self->colours[1], 0x30);

            x += 1;
            y += 1;
        }

        // Preview image?
        if (scenarioInfo->hasFlag(ScenarioIndexFlags::hasPreviewImage))
        {
            const auto imageId = 0;
            const auto g1 = Gfx::getG1Element(imageId);
            if (g1 != nullptr)
            {
                // Temporarily substitute a G1 image with the data in the scenario index
                const auto backupG1 = *g1;
                *g1 = {};
                g1->offset = reinterpret_cast<uint8_t*>(scenarioInfo->preview);
                g1->width = 128;
                g1->height = 128;

                // Draw preview image and restore original G1 image.
                Gfx::drawImage(dpi, x, y, imageId);
                *g1 = backupG1;

                // Draw compass
                Gfx::drawImage(dpi, x, y, ImageIds::height_map_compass);
            }
        }
        else
        {
            x += 1;
            y += 1;

            // No preview image -- a placeholder will have to do.
            auto image = Gfx::recolour(ImageIds::random_map_watermark, self->colours[1]);
            Gfx::drawImage(dpi, x, y, image);

            x += 64;
            y += 59;

            auto args = FormatArguments();
            args.push(StringIds::randomly_generated_landscape);

            // Overlay random map note.
            auto origin = Gfx::point_t(x, y);
            Gfx::drawStringCentredWrapped(dpi, &origin, 128, Colour::black, StringIds::wcolour2_stringid, &args);
        }

        {
            x = baseX;
            y = baseY + 150;

            // Description
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            strncpy(str, scenarioInfo->description, std::size(scenarioInfo->description));

            auto args = FormatArguments();
            args.push(StringIds::buffer_2039);
            y = Gfx::drawString_495224(*dpi, x, y, 170, Colour::black, StringIds::black_stringid, &args);

            // Challenge header
            y += 5;
            auto origin = Gfx::point_t(x, y);
            Gfx::drawString_494B3F(*dpi, &origin, Colour::black, StringIds::challenge_label, nullptr);

            // Challenge text
            str = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            strncpy(str, scenarioInfo->objective, std::size(scenarioInfo->objective));

            y += 10;
            args = FormatArguments();
            args.push(StringIds::buffer_1250);
            y = Gfx::drawString_495224(*dpi, x, y, 170, Colour::black, StringIds::challenge_value, &args);

            // Start year
            y += 5;
            args = FormatArguments();
            args.push(scenarioInfo->startYear);
            Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::challenge_start_date, &args);

            // Competing companies
            y += 10;
            args = FormatArguments();
            args.push<uint16_t>(scenarioInfo->numCompetingCompanies);
            string_id competitionStringId = scenarioInfo->numCompetingCompanies == 0 ? StringIds::challenge_competing_companies_none : StringIds::challenge_competing_companies_up_to;
            y = Gfx::drawString_495224(*dpi, x, y, 170, Colour::black, competitionStringId, &args);

            if (scenarioInfo->numCompetingCompanies == 0 || scenarioInfo->competingCompanyDelay == 0)
                return;

            // Delayed start for competing companies
            args = FormatArguments();
            args.push<uint16_t>(scenarioInfo->competingCompanyDelay);
            competitionStringId = scenarioInfo->numCompetingCompanies == 1 ? StringIds::competition_not_starting_for_month : StringIds::competition_not_starting_for_months;
            Gfx::drawString_494B3F(*dpi, x, y, Colour::black, competitionStringId, &args);
        }
    }

    // 0x00443D02
    static void drawScroll(window* self, Gfx::drawpixelinfo_t* const dpi, uint32_t)
    {
        auto colour = Colour::getShade(self->colours[1], 4);
        Gfx::clearSingle(*dpi, colour);

        using namespace ScenarioManager;
        auto scenarioCount = getScenarioCountByCategory(self->current_tab);

        int16_t y = 0;
        for (auto i = 0; i < scenarioCount; i++)
        {
            auto* scenarioInfo = getNthScenarioFromCategory(self->current_tab, i);
            if (scenarioInfo == nullptr)
                continue;

            if (y + rowHeight < dpi->y || y > dpi->y + dpi->height)
            {
                y += rowHeight;
                continue;
            }

            // Highlight selected item
            auto formatStringId = StringIds::black_stringid;
            if (scenarioInfo == reinterpret_cast<ScenarioIndexEntry*>(self->info))
            {
                Gfx::drawRect(dpi, 0, y, self->width, rowHeight - 1, 0x2000000 | 48);
                formatStringId = StringIds::wcolour2_stringid;
            }

            // Scenario name
            {
                auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
                strncpy(str, scenarioInfo->scenarioName, std::size(scenarioInfo->scenarioName));

                auto args = FormatArguments();
                args.push(StringIds::buffer_2039);

                const int16_t x = 210;
                Gfx::drawStringCentred(*dpi, x, y + 1, Colour::black, formatStringId, &args);
            }

            // Completed?
            if (!scenarioInfo->hasFlag(ScenarioIndexFlags::completed))
            {
                y += rowHeight;
                continue;
            }

            // Draw checkmark to indicate completion
            Gfx::drawImage(dpi, self->widgets[widx::list].width() - ScrollView::barWidth - 25, y + 1, ImageIds::scenario_completed_tick);

            // 'Completed by' info
            {
                auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
                strncpy(str, scenarioInfo->highscoreName, std::size(scenarioInfo->highscoreName));

                auto args = FormatArguments();
                args.push(StringIds::completed_by_name_in_years_months);
                args.push(StringIds::buffer_2039);
                args.push<uint16_t>(scenarioInfo->completedMonths / 12);
                args.push<uint16_t>(scenarioInfo->completedMonths % 12);

                const int16_t x = (self->widgets[widx::list].width() - ScrollView::barWidth) / 2;
                Gfx::drawStringCentred(*dpi, x, y + 10, Colour::black, formatStringId, &args);
            }

            y += rowHeight;
        }
    }

    // 0x00443E9B
    static void onMouseUp(window* self, const widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self);
                break;
        }
    }

    // 0x00443EA6
    static void onMouseDown(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::tab0:
            case widx::tab1:
            case widx::tab2:
            case widx::tab3:
            case widx::tab4:
            {
                uint8_t selectedCategory = widgetIndex - widx::tab0;
                if (self->current_tab == selectedCategory)
                    return;

                self->current_tab = selectedCategory;

                auto& config = Config::get();
                config.scenario_selected_tab = selectedCategory;
                Config::write();

                self->info = 0xFFFFFFFF;
                self->invalidate();
                self->callOnResize();
                self->callPrepareDraw();
                self->initScrollWidgets();
                self->invalidate();

                initList(self);
                break;
            }
        }
    }

    // 0x00443EF6
    static void getScrollSize(window* self, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        *scrollHeight = ScenarioManager::getScenarioCountByCategory(self->current_tab) * rowHeight;
    }

    // 0x00443F32
    static void onScrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto scenarioCount = ScenarioManager::getScenarioCountByCategory(self->current_tab);

        auto index = y / rowHeight;
        if (index > scenarioCount)
            return;

        auto* scenarioInfo = ScenarioManager::getNthScenarioFromCategory(self->current_tab, index);
        if (scenarioInfo == nullptr)
            return;

        // Mouse click sound
        Audio::playSound(Audio::sound_id::click_down, self->x + (self->width / 2));

        if (isNetworked())
        {
            GameCommands::do_67(scenarioInfo->filename);
        }
        else
        {
            Scenario::start(scenarioInfo->filename);
        }
    }

    // 0x00443FB2
    static void onScrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto scenarioCount = ScenarioManager::getScenarioCountByCategory(self->current_tab);

        auto index = y / rowHeight;
        if (index > scenarioCount)
            return;

        auto* scenarioEntry = ScenarioManager::getNthScenarioFromCategory(self->current_tab, index);
        if (scenarioEntry == nullptr)
            return;

        self->info = reinterpret_cast<uintptr_t>(scenarioEntry);
        self->invalidate();
    }

    // 0x00444001
    static std::optional<FormatArguments> tooltip(window* self, widget_index widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_scenario_list);
        return args;
    }

    static void initEvents()
    {
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
        _events.on_mouse_up = onMouseUp;
        _events.on_mouse_down = onMouseDown;
        _events.get_scroll_size = getScrollSize;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
    }
}
