#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/ObjectManager.h"
#include "../ScenarioManager.h"
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

    constexpr auto ROW_HEIGHT = 24;

    static window_event_list _events;

    static void initEvents();

    // 0x00443807
    static void initTabs()
    {
        call(0x00443807);
    }

    // 0x00443946
    static void initList(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x00443946, regs);
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
            window_flags::transparent,
            &_events);

        self->widgets = _widgets;
        self->enabled_widgets = (1 << widx::close) | (1 << widx::tab0) | (1 << widx::tab1) | (1 << widx::tab2) | (1 << widx::tab3) | (1 << widx::tab4);
        self->initScrollWidgets();

        self->colours[0] = PaletteIndex::transparent;
        self->colours[1] = PaletteIndex::index_0B;

        self->var_846 = 0xFFFF;
        self->var_85A = static_cast<int32_t>(0xFFFFFFFF);

        initTabs();

        // auto& config = config::get();
        // config.scenario_selected_tab;

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
        Gfx::drawRectInset(dpi, self->x, self->y + 20, self->width - 1, 1, self->colours[0], 0);

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

        using namespace scenariomgr;
        auto scenarioInfo = reinterpret_cast<ScenarioIndexEntry*>(self->info);

        // Load currency object.
        // TODO loc_443A5F
        // objectmgr::getLoadedObjectIndex(self->object);

        // loc_443A8A
        // Scenario name
        {
            const int16_t x = self->x + self->widgets[widx::list].right + 89;
            const int16_t y = self->y + self->widgets[widx::panel].top + 5;

            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            strncpy(str, scenarioInfo->scenarioName, std::size(scenarioInfo->scenarioName));

            auto args = FormatArguments();
            args.push(StringIds::buffer_2039);
            Gfx::drawStringCentredClipped(*dpi, x, y, 170, Colour::black, StringIds::wcolour2_stringid, &args);
        }

        // Outline for preview image
        {
            const int16_t x = self->x + self->widgets[widx::list].right + 24;
            const int16_t y = self->y + self->widgets[widx::panel].top + 19;

            Gfx::drawRectInset(dpi, x, y, 130, 130, self->colours[1], 0x30);
        }

        // Preview image?
        if (scenarioInfo->hasFlag(ScenarioIndexFlags::hasPreviewImage))
        {
            const int16_t x = self->x + self->widgets[widx::list].right + 25;
            const int16_t y = self->y + self->widgets[widx::panel].top + 20;

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
            }
        }
        else
        {
            int16_t x = self->x + self->widgets[widx::list].right + 25;
            int16_t y = self->y + self->widgets[widx::panel].top + 21;

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
    }

    // 0x00443D02
    static void drawScroll(window*, Gfx::drawpixelinfo_t* const dpi, uint32_t)
    {
    }

    // 0x00443E9B
    static void onMouseUp(window* self, const widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self->type);
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
                self->current_tab = selectedCategory;

                auto& config = Config::get();
                config.scenario_selected_tab = selectedCategory;
                Config::write();

                self->object = reinterpret_cast<char*>(0xFFFFFFFF);
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
        *scrollHeight = scenariomgr::getNumScenariosByCategory(self->current_tab) * ROW_HEIGHT;
    }

    // 0x00443F32
    static void onScrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto numScenarios = scenariomgr::getNumScenariosByCategory(self->current_tab);
    }

    // 0x00443FB2
    static void onScrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto numScenarios = scenariomgr::getNumScenariosByCategory(self->current_tab);

        auto index = y / ROW_HEIGHT;
        if (index > numScenarios)
            return;

        auto* scenarioEntry = scenariomgr::getNthScenarioFromCategory(self->current_tab, index);
        if (scenarioEntry == nullptr)
            return;

        self->info = reinterpret_cast<uintptr_t>(scenarioEntry);
        self->invalidate();
    }

    // 0x00444001
    static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
    {
        args.push(StringIds::tooltip_scroll_scenario_list);
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
