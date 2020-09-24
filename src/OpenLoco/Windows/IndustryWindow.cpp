#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Date.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Graphics/Types.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CargoObject.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::ui::windows::industry
{
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_industry,
            tab_production,
            tab_production_2,
            tab_transported,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_industry) | (1 << widx::tab_production) | (1 << widx::tab_production_2) | (1 << widx::tab_transported);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                        \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                          \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                     \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 95 }, widget_type::panel, 1),                                                              \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_industry),                      \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_production_graph),             \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_production_graph),             \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_statistics)

        // Defined at the bottom of this file.
        static void prepareDraw(window* self);
        static void textInput(window* self, widget_index callingWidget, char* input);
        static void update(window* self);
        static void renameIndustryPrompt(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi);
        static void setDisabledWidgets(window* self);
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi);
        static void onMouseUp(window* self, widget_index widgetIndex);
        static void initEvents();
    }

    namespace industry
    {
        static const Gfx::ui_size_t windowSize = { 223, 137 };

        static const Gfx::ui_size_t minWindowSize = { 192, 137 };

        static const Gfx::ui_size_t maxWindowSize = { 600, 440 };

        enum widx
        {
            viewport = 8,
            status_bar,
            centre_on_viewport,
            demolish_industry,
        };

        static widget_t widgets[] = {
            commonWidgets(223, 137, StringIds::title_town),
            makeWidget({ 3, 44 }, { 195, 80 }, widget_type::viewport, 1, 0xFFFFFFFE),
            makeWidget({ 3, 115 }, { 195, 21 }, widget_type::wt_13, 1),
            makeWidget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::move_main_view_to_show_this),
            makeWidget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::rubbish_bin, StringIds::demolish_this_industry),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << centre_on_viewport) | (1 << demolish_industry);

        static window_event_list events;

        // 0x00455ADD
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::demolish_industry].right = self->width - 2;
            self->widgets[widx::demolish_industry].left = self->width - 25;

            if (isEditorMode())
            {
                self->widgets[widx::demolish_industry].type = widget_type::wt_9;
            }
            else
            {
                self->widgets[widx::demolish_industry].type = widget_type::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x00455C22
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            self->drawViewports(dpi);
            widget::drawViewportCentreButton(dpi, self, widx::centre_on_viewport);

            const char* buffer = StringManager::getString(StringIds::buffer_1250);
            auto industry = industrymgr::get(self->number);
            industry->getStatusString(const_cast<char*>(buffer));

            auto args = FormatArguments();
            args.push(StringIds::buffer_1250);

            auto widget = &self->widgets[widx::status_bar];
            auto x = self->x + widget->left - 1;
            auto y = self->y + widget->top - 1;
            auto width = widget->width();
            Gfx::drawString_494BBF(*dpi, x, y, width, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x00455C86
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameIndustryPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_industry:
                case common::widx::tab_production:
                case common::widx::tab_production_2:
                case common::widx::tab_transported:
                    common::switchTab(self, widgetIndex);
                    break;

                // 0x00455EA2
                case widx::centre_on_viewport:
                {
                    if (self->viewports[0] == nullptr || self->saved_view.isEmpty())
                        break;

                    auto main = WindowManager::getMainWindow();
                    main->viewportCentreOnTile(self->saved_view.getPos());
                    break;
                }

                // 0x00455E59
                case widx::demolish_industry:
                {
                    bool success = GameCommands::do_48(self->number);
                    if (!success)
                        break;

                    loco_global<uint16_t, 0x009C68E0> gameCommandMapX;
                    loco_global<uint16_t, 0x009C68E2> gameCommandMapY;
                    loco_global<uint16_t, 0x009C68E4> gameCommandMapZ;

                    Audio::playSound(Audio::sound_id::demolish, loc16(gameCommandMapX, gameCommandMapY, gameCommandMapZ));
                    break;
                }
            }
        }

        static void initViewport(window* self);

        // 0x00455F1A
        static void onResize(window* self)
        {
            self->setSize(minWindowSize, maxWindowSize);

            if (self->viewports[0] != nullptr)
            {
                uint16_t newWidth = self->width - 30;
                if (!isEditorMode())
                    newWidth += 22;

                uint16_t newHeight = self->height - 59;

                auto& viewport = self->viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->view_width = newWidth << viewport->zoom;
                    viewport->view_height = newHeight << viewport->zoom;
                    self->saved_view.clear();
                }
            }

            initViewport(self);
        }

        // 0x00456C36
        static void initViewport(window* self)
        {
            if (self->current_tab != common::widx::tab_industry - common::widx::tab_industry)
                return;

            self->callPrepareDraw();

            // Figure out the industry's position on the map.
            auto industry = industrymgr::get(self->number);
            int16_t tileZ = OpenLoco::Map::tileElementHeight(industry->x, industry->y).landHeight;

            // Compute views.
            SavedView view = {
                industry->x,
                industry->y,
                ZoomLevel::quarter,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                tileZ,
            };
            //view.flags |= (1 << 14);

            uint16_t flags = 0;
            if (self->viewports[0] != nullptr)
            {
                if (self->saved_view == view)
                    return;

                flags = self->viewports[0]->flags;
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                viewportmgr::collectGarbage();
            }
            else
            {
                if ((config::get().flags & config::flags::gridlines_on_landscape) != 0)
                    flags |= viewport_flags::gridlines_on_landscape;
            }

            self->saved_view = view;

            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = OpenLoco::Map::map_pos3({ industry->x, industry->y, tileZ });
                auto origin = Gfx::point_t(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = Gfx::ui_size_t(widget->width() - 2, widget->height() - 2);
                viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, tile);
                self->invalidate();
                self->flags |= window_flags::viewport_no_scrolling;
            }

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->flags = flags;
                self->invalidate();
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
            events.viewport_rotate = initViewport;
        }
    }

    // 0x00456D2D
    window* open(industry_id_t industryId)
    {
        auto window = WindowManager::bringToFront(WindowType::industry, industryId);
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
                Input::toolCancel();

            window = WindowManager::bringToFront(WindowType::industry, industryId);
        }

        if (window == nullptr)
        {
            // 0x00456DBC start
            const uint32_t newFlags = window_flags::flag_8 | window_flags::resizable;
            window = WindowManager::createWindow(WindowType::industry, industry::windowSize, newFlags, &industry::events);
            window->number = industryId;
            window->min_width = 192;
            window->min_height = 137;
            window->max_width = 600;
            window->max_height = 440;

            auto skin = ObjectManager::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_0B;
                window->colours[1] = skin->colour_0C;
            }
            // 0x00456DBC end

            window->saved_view.clear();
        }

        // TODO: only needs to be called once.
        common::initEvents();

        window->current_tab = common::widx::tab_industry - common::widx::tab_industry;
        window->invalidate();

        window->widgets = industry::widgets;
        window->enabled_widgets = industry::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &industry::events;
        window->activated_widgets = 0;

        common::setDisabledWidgets(window);

        window->initScrollWidgets();
        industry::initViewport(window);

        return window;
    }

    namespace production
    {

        static const Gfx::ui_size_t minWindowSize = { 299, 282 };

        static const Gfx::ui_size_t maxWindowSize = { 299, 337 };

        static window_event_list events;

        // 0x00455FD9
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            common::repositionTabs(self);
        }

        // 0x0045654F
        static void onResize(window* self)
        {
            {
                self->setSize(minWindowSize, maxWindowSize);
            }
        }

        static void initEvents()
        {
            events.draw = common::draw;
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
        }
    }

    namespace production_2
    {
        static const Gfx::ui_size_t minWindowSize = { 299, 282 };

        static const Gfx::ui_size_t maxWindowSize = { 299, 337 };

        static widget_t widgets[] = {
            commonWidgets(222, 136, StringIds::title_industry_monthly_production),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0045626F
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            common::repositionTabs(self);
        }

        // 0x004565FF
        static void onResize(window* self)
        {
            {
                self->setSize(minWindowSize, maxWindowSize);
            }
        }

        static void initEvents()
        {
            events.draw = common::draw;
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
        }
    }

    namespace transported
    {
        static const Gfx::ui_size_t windowSize = { 300, 127 };

        static widget_t widgets[] = {
            commonWidgets(300, 126, StringIds::title_statistics),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x00456665
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            common::repositionTabs(self);
        }

        // 0x00456705
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto industry = industrymgr::get(self->number);
            auto industryObj = industry->object();
            int16_t xPos = self->x + 3;
            int16_t yPos = self->y + 45;
            Gfx::point_t origin = { xPos, yPos };

            // Draw Last Months received cargo stats
            if (industry->canReceiveCargo())
            {
                origin.x += 4;
                origin.y += 10;
                Gfx::drawString_494B3F(*dpi, xPos, yPos, Colour::black, StringIds::received_cargo);

                auto cargoNumber = 0;
                for (const auto& receivedCargoType : industryObj->required_cargo_type)
                {
                    if (receivedCargoType != 0xFF)
                    {
                        auto cargoObj = ObjectManager::get<cargo_object>(receivedCargoType);
                        auto args = FormatArguments();

                        if (industry->required_cargo_quantity[cargoNumber] == 1)
                        {
                            args.push(cargoObj->unit_name_singular);
                        }
                        else
                        {
                            args.push(cargoObj->unit_name_plural);
                        }
                        args.push<uint32_t>(industry->required_cargo_quantity[cargoNumber]);

                        origin.y = Gfx::drawString_495224(*dpi, origin.x, origin.y, 290, Colour::black, StringIds::black_stringid, &args);
                    }
                    cargoNumber++;
                }
                origin.y += 4;
                origin.x -= 4;
            }

            // Draw Last Months produced cargo stats
            if (industry->canProduceCargo())
            {
                Gfx::drawString_494B3F(*dpi, origin.x, origin.y, Colour::black, StringIds::produced_cargo);
                origin.y += 10;
                origin.x += 4;

                auto cargoNumber = 0;
                for (const auto& producedCargoType : industryObj->produced_cargo_type)
                {
                    if (producedCargoType != 0xFF)
                    {
                        auto cargoObj = ObjectManager::get<cargo_object>(producedCargoType);
                        auto args = FormatArguments();

                        if (industry->produced_cargo_quantity[cargoNumber] == 1)
                        {
                            args.push(cargoObj->unit_name_singular);
                        }
                        else
                        {
                            args.push(cargoObj->unit_name_plural);
                        }
                        args.push<uint32_t>(industry->produced_cargo_quantity[cargoNumber]);
                        args.push<uint16_t>(industry->produced_cargo_transported[cargoNumber]);

                        origin.y = Gfx::drawString_495224(*dpi, origin.x, origin.y, 290, Colour::black, StringIds::transported_cargo, &args);
                    }
                    cargoNumber++;
                }
            }
        }

        // 0x004569C2
        static void onResize(window* self)
        {
            {
                self->setSize(windowSize, windowSize);
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
        }
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t* enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { industry::widgets, widx::tab_industry, &industry::events, &industry::enabledWidgets },
            { production_2::widgets, widx::tab_production, &production::events, &common::enabledWidgets },
            { production_2::widgets, widx::tab_production_2, &production_2::events, &common::enabledWidgets },
            { transported::widgets, widx::tab_transported, &transported::events, &common::enabledWidgets }
        };

        static void setDisabledWidgets(window* self)
        {
            auto industryObj = ObjectManager::get<industry_object>(industrymgr::get(self->number)->object_id);
            auto disabledWidgets = 0;

            if (industryObj->produced_cargo_type[0] == 0xFF)
                disabledWidgets |= (1 << common::widx::tab_production);

            if (industryObj->produced_cargo_type[1] == 0xFF)
                disabledWidgets |= (1 << common::widx::tab_production_2);

            self->disabled_widgets = disabledWidgets;
        }

        // 0x00456079
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            // Draw Units of Cargo sub title
            const auto industry = industrymgr::get(self->number);
            const auto industryObj = ObjectManager::get<industry_object>(industry->object_id);
            const auto cargoObj = ObjectManager::get<cargo_object>(industryObj->produced_cargo_type[0]);

            {
                auto args = FormatArguments();
                args.push(cargoObj->units_and_cargo_name);

                int16_t x = self->x + 2;
                int16_t y = self->y - 24 + 68;

                Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::production_graph_label, &args);
            }

            // Draw Y label and grid lines.
            const uint16_t graphBottom = self->y + self->height - 7;
            int32_t yTick = 0;
            for (int16_t yPos = graphBottom; yPos >= self->y + 68; yPos -= 20)
            {
                auto args = FormatArguments();
                args.push(yTick);

                Gfx::drawRect(dpi, self->x + 41, yPos, 239, 1, Colour::getShade(self->colours[1], 4));

                Gfx::drawString_494C78(*dpi, self->x + 39, yPos - 6, Colour::black, StringIds::population_graph_people, &args);

                yTick += 1000;
            }

            month_id month = getCurrentMonth();
            int16_t year = getCurrentYear();
            int8_t yearSkip = 0;
            // This is either 0 or 1 depending on selected tab
            // used to select the correct history
            const uint8_t productionTabWidx = self->current_tab + widx::tab_industry;
            const uint8_t productionNum = productionTabWidx - widx::tab_production;
            for (uint8_t i = industry->history_size[productionNum] - 1; i > 0; i--)
            {
                const uint16_t xPos = self->x + 41 + i;
                const uint16_t yPos = self->y + 56;

                // Draw horizontal year and vertical grid lines.
                if (month == month_id::january)
                {
                    if (yearSkip == 0)
                    {
                        auto args = FormatArguments();
                        args.push(year);

                        Gfx::drawStringCentred(*dpi, xPos, yPos, Colour::black, StringIds::population_graph_year, &args);
                    }

                    Gfx::drawRect(dpi, xPos, yPos + 11, 1, self->height - 74, Colour::getShade(self->colours[1], 4));
                }

                const auto history = productionTabWidx == widx::tab_production ? industry->history_1 : industry->history_2;
                // Draw production graph
                const uint16_t yPos1 = graphBottom - history[i];
                const uint16_t yPos2 = graphBottom - history[i + 1];

                // Do not draw current segment yet; it may be zeroed.
                if (i < industry->history_size[productionNum] - 1)
                {
                    if (yPos1 <= graphBottom)
                    {
                        if (yPos2 <= graphBottom)
                        {
                            Gfx::drawLine(dpi, xPos, yPos1, xPos + 1, yPos2, Colour::getShade(self->colours[1], 7));
                        }
                    }
                }

                if (month == month_id::january)
                {
                    month = month_id::december;
                    year--;

                    yearSkip++;
                    if (yearSkip >= 3)
                        yearSkip = 0;
                }
                else
                {
                    month = month_id(static_cast<int8_t>(month) - 1);
                }
            }
        }

        // 0x004565B5, 0x00456505
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameIndustryPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_industry:
                case common::widx::tab_production:
                case common::widx::tab_production_2:
                case common::widx::tab_transported:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static void prepareDraw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1ULL << widx::tab_industry) | (1ULL << widx::tab_production) | (1ULL << widx::tab_production_2) | (1ULL << widx::tab_transported));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Put industry name in place.
            auto industry = industrymgr::get(self->number);
            auto args = FormatArguments();
            args.push(industry->name);
            args.push(industry->town);

            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        // 0x00455CBC
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget != common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            gGameCommandErrorTitle = StringIds::error_cant_rename_industry;

            uint32_t* buffer = (uint32_t*)input;
            GameCommands::do_79(self->number, 1, buffer[0], buffer[1], buffer[2]);
            GameCommands::do_79(0, 2, buffer[3], buffer[4], buffer[5]);
            GameCommands::do_79(0, 0, buffer[6], buffer[7], buffer[8]);
        }

        static void update(window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::industry, self->number);
        }

        //0x00455D81
        static void renameIndustryPrompt(window* self, widget_index widgetIndex)
        {
            auto industry = industrymgr::get(self->number);
            if (!isEditorMode())
            {
                if ((industry->flags & industry_flags::flag_04) == 0)
                    return;
                if (!isPlayerCompany(industry->owner))
                    return;
            }

            auto args = FormatArguments();
            args.push<int64_t>(0);
            args.push(industry->name);
            args.push(industry->town);

            textinput::openTextinput(self, StringIds::title_industry_name, StringIds::prompt_enter_new_industry_name, industry->name, widgetIndex, &industry->town);
        }

        // 0x00456A5E, 0x00456A64
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_industry].left;
            const int16_t tabWidth = self->widgets[widx::tab_industry].right - xPos;

            for (uint8_t i = widx::tab_industry; i <= widx::tab_transported; i++)
            {
                if (self->isDisabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x00455CC7
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            ui::textinput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_industry;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);
            self->var_85C = -1;

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_industry];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            common::setDisabledWidgets(self);

            self->invalidate();

            self->setSize(industry::windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        static void drawProductionTab(window* self, Gfx::drawpixelinfo_t* dpi, uint8_t productionTabNumber)
        {
            static const uint32_t productionTabImageIds[] = {
                InterfaceSkin::ImageIds::tab_production_frame0,
                InterfaceSkin::ImageIds::tab_production_frame1,
                InterfaceSkin::ImageIds::tab_production_frame2,
                InterfaceSkin::ImageIds::tab_production_frame3,
                InterfaceSkin::ImageIds::tab_production_frame4,
                InterfaceSkin::ImageIds::tab_production_frame5,
                InterfaceSkin::ImageIds::tab_production_frame6,
                InterfaceSkin::ImageIds::tab_production_frame7,
            };

            auto industry = industrymgr::get(self->number);
            auto industryObj = ObjectManager::get<industry_object>(industry->object_id);
            auto skin = ObjectManager::get<interface_skin_object>();

            static const uint32_t productionTabIds[] = {
                widx::tab_production,
                widx::tab_production_2,
            };

            auto tab = productionTabIds[productionTabNumber];

            uint32_t imageId = 0xFFFFFFFF;
            auto widget = self->widgets[tab];

            if (industryObj->produced_cargo_type[productionTabNumber] != 0xFF)
            {
                imageId = Gfx::recolour(skin->img, self->colours[1]);

                if (self->current_tab == tab - widx::tab_industry)
                    imageId += productionTabImageIds[(self->frame_no / 4) % std::size(productionTabImageIds)];
                else
                    imageId += productionTabImageIds[0];

                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                Gfx::drawImage(dpi, xPos, yPos, imageId);

                auto caroObj = ObjectManager::get<cargo_object>(industryObj->produced_cargo_type[productionTabNumber]);
                Gfx::drawImage(dpi, xPos + 18, yPos + 14, caroObj->unit_inline_sprite);

                widget::draw_tab(self, dpi, -2, tab);
            }
        }

        // 0x00456A98
        static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            auto skin = ObjectManager::get<interface_skin_object>();

            // Industry tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_industries;
                widget::draw_tab(self, dpi, imageId, widx::tab_industry);
            }

            // Production Tab
            {
                drawProductionTab(self, dpi, 0);
            }

            // 2nd Production Tab
            {
                drawProductionTab(self, dpi, 1);
            }

            // Transported Tab
            {
                static const uint32_t transportedTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_transported_frame0,
                    InterfaceSkin::ImageIds::tab_transported_frame1,
                    InterfaceSkin::ImageIds::tab_transported_frame2,
                    InterfaceSkin::ImageIds::tab_transported_frame3,
                    InterfaceSkin::ImageIds::tab_transported_frame4,
                    InterfaceSkin::ImageIds::tab_transported_frame5,
                    InterfaceSkin::ImageIds::tab_transported_frame6,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_transported - widx::tab_industry)
                    imageId += transportedTabImageIds[(self->frame_no / 4) % std::size(transportedTabImageIds)];
                else
                    imageId += transportedTabImageIds[0];
                widget::draw_tab(self, dpi, imageId, widx::tab_transported);
            }
        }

        static void initEvents()
        {
            industry::initEvents();
            production::initEvents();
            production_2::initEvents();
            transported::initEvents();
        }
    }
}
