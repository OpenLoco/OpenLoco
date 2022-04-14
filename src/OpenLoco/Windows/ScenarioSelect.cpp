#include "../Audio/Audio.h"
#include "../Config.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/ObjectManager.h"
#include "../Scenario.h"
#include "../ScenarioManager.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ScenarioSelect
{
    constexpr Ui::Size windowSize = { 610, 412 };

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

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 610, 412 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 608, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::select_scenario_for_new_game),
        makeWidget({ 595, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 48 }, { 610, 364 }, WidgetType::wt_3, WindowColour::secondary),
        makeRemapWidget({ 3, 15 }, { 91, 34 }, WidgetType::tab, WindowColour::secondary, ImageIds::wide_tab),
        makeRemapWidget({ 94, 15 }, { 91, 34 }, WidgetType::tab, WindowColour::secondary, ImageIds::wide_tab),
        makeRemapWidget({ 185, 15 }, { 91, 34 }, WidgetType::tab, WindowColour::secondary, ImageIds::wide_tab),
        makeRemapWidget({ 276, 15 }, { 91, 34 }, WidgetType::tab, WindowColour::secondary, ImageIds::wide_tab),
        makeRemapWidget({ 367, 15 }, { 91, 34 }, WidgetType::tab, WindowColour::secondary, ImageIds::wide_tab),
        makeWidget({ 3, 52 }, { 431, 356 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        widgetEnd(),
    };

    constexpr auto rowHeight = 24;

    static WindowEventList _events;

    static void initEvents();

    // 0x00443807
    static void initTabs(Window* self)
    {
        uint16_t xPos = 3;
        for (int i = 0; i < 5; i++)
        {
            Widget& widget = self->widgets[widx::tab0 + i];
            if (ScenarioManager::hasScenariosForCategory(i))
            {
                widget.type = WidgetType::tab;
                widget.left = xPos;
                widget.right = xPos + 90;
                xPos += 91;
            }
            else
                widget.type = WidgetType::none;
        }
    }

    // 0x00443946
    static void initList(Window* self)
    {
        if (self->info == 0xFFFFFFFF)
            return;

        using namespace ScenarioManager;
        auto scenarioInfo = reinterpret_cast<ScenarioIndexEntry*>(self->info);
        if (hasScenarioInCategory(self->currentTab, scenarioInfo))
            return;

        // Reset currently selected scenario if it is not in the current category.
        self->info = 0xFFFFFFFF;
        self->invalidate();
    }

    // 0x00443868
    Window* open()
    {
        Window* self = WindowManager::bringToFront(WindowType::scenarioSelect);
        if (self != nullptr)
            return self;

        initEvents();

        self = WindowManager::createWindow(
            WindowType::scenarioSelect,
            Ui::Point({ static_cast<int16_t>(width() / 2 - windowSize.width / 2),
                        std::max<int16_t>(height() / 2 - windowSize.height / 2, 28) }),
            windowSize,
            WindowFlags::stickToFront | WindowFlags::flag_12,
            &_events);

        self->widgets = _widgets;
        self->enabledWidgets = (1 << widx::close) | (1 << widx::tab0) | (1 << widx::tab1) | (1 << widx::tab2) | (1 << widx::tab3) | (1 << widx::tab4);
        self->initScrollWidgets();

        self->setColour(WindowColour::primary, Colour::black);
        self->setColour(WindowColour::secondary, Colour::mutedSeaGreen);

        self->var_846 = 0xFFFF;
        self->var_85A = static_cast<int32_t>(0xFFFFFFFF);

        initTabs(self);

        // Select the last tab used, or the first available one.
        uint8_t selectedTab = Config::get().scenarioSelectedTab;
        if (self->widgets[widx::tab0 + selectedTab].type == WidgetType::none)
        {
            selectedTab = 0;
            for (int i = 0; i < 5; i++)
            {
                if (self->widgets[widx::tab0 + i].type == WidgetType::none)
                {
                    selectedTab = i;
                    break;
                }
            }
        }

        self->currentTab = selectedTab;

        initList(self);

        return self;
    }

    // 0x00443995
    static void prepareDraw(Window* self)
    {
        self->activatedWidgets &= ~((1 << widx::tab0) | (1 << widx::tab1) | (1 << widx::tab2) | (1 << widx::tab3) | (1 << widx::tab4));
        self->activatedWidgets |= (1ULL << (self->currentTab + static_cast<uint8_t>(widx::tab0)));
    }

    // 0x004439AF
    static void draw(Window* self, Gfx::Context* context)
    {
        Gfx::drawRectInset(*context, self->x, self->y + 20, self->width, 41, self->getColour(WindowColour::primary).u8(), 0);

        // Draw widgets.
        self->draw(context);

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
            Widget& widget = self->widgets[widx::tab0 + i];
            if (widget.type == WidgetType::none)
                continue;

            const auto offset = self->currentTab == i ? 1 : 0;
            auto origin = Ui::Point(widget.mid_x() + self->x, widget.mid_y() + self->y - 3 - offset);
            const string_id caption = scenarioGroupIds[i];

            Gfx::drawStringCentredWrapped(*context, origin, widget.width() - 4, Colour::black, StringIds::wcolour2_stringid, &caption);
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
            // Swap out the currency object and reload
            ObjectManager::unload(ObjectManager::getHeader({ ObjectType::currency, 0 }));
            ObjectManager::load(scenarioInfo->currency);
            ObjectManager::reloadAll();
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
            Gfx::drawStringCentredClipped(*context, x, y, 170, Colour::black, StringIds::wcolour2_stringid, &args);

            y += 14;
        }

        // Outline for preview image
        {
            x = baseX + 20;
            Gfx::drawRectInset(*context, x, y, 130, 130, self->getColour(WindowColour::secondary).u8(), 0x30);

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
                Gfx::drawImage(context, x, y, imageId);
                *g1 = backupG1;

                // Draw compass
                Gfx::drawImage(context, x, y, ImageIds::height_map_compass);
            }
        }
        else
        {
            x += 1;
            y += 1;

            // No preview image -- a placeholder will have to do.
            auto image = Gfx::recolour(ImageIds::random_map_watermark, self->getColour(WindowColour::secondary).c());
            Gfx::drawImage(context, x, y, image);

            x += 64;
            y += 59;

            auto args = FormatArguments();
            args.push(StringIds::randomly_generated_landscape);

            // Overlay random map note.
            auto origin = Ui::Point(x, y);
            Gfx::drawStringCentredWrapped(*context, origin, 128, Colour::black, StringIds::wcolour2_stringid, &args);
        }

        {
            x = baseX;
            y = baseY + 150;

            // Description
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            strncpy(str, scenarioInfo->description, std::size(scenarioInfo->description));

            auto args = FormatArguments();
            args.push(StringIds::buffer_2039);
            y = Gfx::drawString_495224(*context, x, y, 170, Colour::black, StringIds::black_stringid, &args);

            // Challenge header
            y += 5;
            auto origin = Ui::Point(x, y);
            Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::challenge_label, nullptr);

            // Challenge text
            str = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            strncpy(str, scenarioInfo->objective, std::size(scenarioInfo->objective));

            y += 10;
            args = FormatArguments();
            args.push(StringIds::buffer_1250);
            y = Gfx::drawString_495224(*context, x, y, 170, Colour::black, StringIds::challenge_value, &args);

            // Start year
            y += 5;
            args = FormatArguments();
            args.push(scenarioInfo->startYear);
            Gfx::drawString_494B3F(*context, x, y, Colour::black, StringIds::challenge_start_date, &args);

            // Competing companies
            y += 10;
            args = FormatArguments();
            args.push<uint16_t>(scenarioInfo->numCompetingCompanies);
            string_id competitionStringId = scenarioInfo->numCompetingCompanies == 0 ? StringIds::challenge_competing_companies_none : StringIds::challenge_competing_companies_up_to;
            y = Gfx::drawString_495224(*context, x, y, 170, Colour::black, competitionStringId, &args);

            if (scenarioInfo->numCompetingCompanies == 0 || scenarioInfo->competingCompanyDelay == 0)
                return;

            // Delayed start for competing companies
            args = FormatArguments();
            args.push<uint16_t>(scenarioInfo->competingCompanyDelay);
            competitionStringId = scenarioInfo->numCompetingCompanies == 1 ? StringIds::competition_not_starting_for_month : StringIds::competition_not_starting_for_months;
            Gfx::drawString_494B3F(*context, x, y, Colour::black, competitionStringId, &args);
        }
    }

    // 0x00443D02
    static void drawScroll(Window& self, Gfx::Context& context, const uint32_t)
    {
        auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 4);
        Gfx::clearSingle(context, colour);

        using namespace ScenarioManager;
        auto scenarioCount = getScenarioCountByCategory(self.currentTab);

        int16_t y = 0;
        for (auto i = 0; i < scenarioCount; i++)
        {
            auto* scenarioInfo = getNthScenarioFromCategory(self.currentTab, i);
            if (scenarioInfo == nullptr)
                continue;

            if (y + rowHeight < context.y || y > context.y + context.height)
            {
                y += rowHeight;
                continue;
            }

            // Highlight selected item
            auto formatStringId = StringIds::black_stringid;
            if (scenarioInfo == reinterpret_cast<ScenarioIndexEntry*>(self.info))
            {
                Gfx::drawRect(context, 0, y, self.width, rowHeight - 1, 0x2000000 | 48);
                formatStringId = StringIds::wcolour2_stringid;
            }

            // Scenario name
            {
                auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
                strncpy(str, scenarioInfo->scenarioName, std::size(scenarioInfo->scenarioName));

                auto args = FormatArguments();
                args.push(StringIds::buffer_2039);

                const int16_t x = 210;
                Gfx::drawStringCentred(context, x, y + 1, Colour::black, formatStringId, &args);
            }

            // Completed?
            if (!scenarioInfo->hasFlag(ScenarioIndexFlags::completed))
            {
                y += rowHeight;
                continue;
            }

            // Draw checkmark to indicate completion
            Gfx::drawImage(&context, self.widgets[widx::list].width() - ScrollView::barWidth - 25, y + 1, ImageIds::scenario_completed_tick);

            // 'Completed by' info
            {
                auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
                strncpy(str, scenarioInfo->highscoreName, std::size(scenarioInfo->highscoreName));

                auto args = FormatArguments();
                args.push(StringIds::completed_by_name_in_years_months);
                args.push(StringIds::buffer_2039);
                args.push<uint16_t>(scenarioInfo->completedMonths / 12);
                args.push<uint16_t>(scenarioInfo->completedMonths % 12);

                const int16_t x = (self.widgets[widx::list].width() - ScrollView::barWidth) / 2;
                Gfx::drawStringCentred(context, x, y + 10, Colour::black, formatStringId, &args);
            }

            y += rowHeight;
        }
    }

    // 0x00443E9B
    static void onMouseUp(Window* self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self);
                break;
        }
    }

    // 0x00443EA6
    static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
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
                if (self->currentTab == selectedCategory)
                    return;

                self->currentTab = selectedCategory;

                auto& config = Config::get();
                config.scenarioSelectedTab = selectedCategory;
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
    static void getScrollSize(Window* self, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        *scrollHeight = ScenarioManager::getScenarioCountByCategory(self->currentTab) * rowHeight;
    }

    // 0x00443F32
    static void onScrollMouseDown(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto scenarioCount = ScenarioManager::getScenarioCountByCategory(self->currentTab);

        auto index = y / rowHeight;
        if (index > scenarioCount)
            return;

        auto* scenarioInfo = ScenarioManager::getNthScenarioFromCategory(self->currentTab, index);
        if (scenarioInfo == nullptr)
            return;

        // Mouse click sound
        Audio::playSound(Audio::SoundId::clickDown, self->x + (self->width / 2));

        if (isNetworked())
        {
            GameCommands::do_67(scenarioInfo->filename);
        }
        else
        {
            Scenario::loadAndStart(scenarioInfo->filename);
        }
    }

    // 0x00443FB2
    static void onScrollMouseOver(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto scenarioCount = ScenarioManager::getScenarioCountByCategory(self->currentTab);

        auto index = y / rowHeight;
        if (index > scenarioCount)
            return;

        auto* scenarioEntry = ScenarioManager::getNthScenarioFromCategory(self->currentTab, index);
        if (scenarioEntry == nullptr)
            return;

        self->info = reinterpret_cast<uintptr_t>(scenarioEntry);
        self->invalidate();
    }

    // 0x00444001
    static std::optional<FormatArguments> tooltip(Window* self, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_scenario_list);
        return args;
    }

    static void initEvents()
    {
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
        _events.onMouseUp = onMouseUp;
        _events.onMouseDown = onMouseDown;
        _events.getScrollSize = getScrollSize;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
    }
}
