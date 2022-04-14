#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Date.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Map.hpp"
#include "../Map/TileManager.h"
#include "../Objects/CargoObject.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Ui::Windows::Industry
{
    namespace Common
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 95 }, WidgetType::panel, WindowColour::secondary),                                                                       \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_industry),                                \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_production_graph),                       \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_production_graph),                       \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_statistics)

        // Defined at the bottom of this file.
        static void prepareDraw(Window* self);
        static void textInput(Window* self, WidgetIndex_t callingWidget, const char* input);
        static void update(Window* self);
        static void renameIndustryPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void repositionTabs(Window* self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void setDisabledWidgets(Window* self);
        static void draw(Window* self, Gfx::Context* context);
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex);
        static void initEvents();
    }

    namespace Industry
    {
        static const Ui::Size windowSize = { 223, 137 };

        static const Ui::Size minWindowSize = { 192, 137 };

        static const Ui::Size maxWindowSize = { 600, 440 };

        enum widx
        {
            viewport = 8,
            status_bar,
            centre_on_viewport,
            demolish_industry,
        };

        static Widget widgets[] = {
            commonWidgets(223, 137, StringIds::title_town),
            makeWidget({ 3, 44 }, { 195, 80 }, WidgetType::viewport, WindowColour::secondary, 0xFFFFFFFE),
            makeWidget({ 3, 115 }, { 195, 21 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::null, StringIds::move_main_view_to_show_this),
            makeWidget({ 198, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rubbish_bin, StringIds::demolish_this_industry),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << centre_on_viewport) | (1 << demolish_industry);

        static WindowEventList events;

        // 0x00455ADD
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::demolish_industry].right = self->width - 2;
            self->widgets[widx::demolish_industry].left = self->width - 25;

            if (isEditorMode() || isSandboxMode())
            {
                self->widgets[widx::demolish_industry].type = WidgetType::buttonWithImage;
            }
            else
            {
                self->widgets[widx::demolish_industry].type = WidgetType::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            Common::repositionTabs(self);
        }

        // 0x00455C22
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
            self->drawViewports(context);
            Widget::drawViewportCentreButton(context, self, widx::centre_on_viewport);

            const char* buffer = StringManager::getString(StringIds::buffer_1250);
            auto industry = IndustryManager::get(IndustryId(self->number));
            industry->getStatusString(const_cast<char*>(buffer));

            auto args = FormatArguments();
            args.push(StringIds::buffer_1250);

            auto widget = &self->widgets[widx::status_bar];
            auto x = self->x + widget->left - 1;
            auto y = self->y + widget->top - 1;
            auto width = widget->width();
            Gfx::drawString_494BBF(*context, x, y, width, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x00455C86
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameIndustryPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_industry:
                case Common::widx::tab_production:
                case Common::widx::tab_production_2:
                case Common::widx::tab_transported:
                    Common::switchTab(self, widgetIndex);
                    break;

                // 0x00455EA2
                case widx::centre_on_viewport:
                    self->viewportCentreMain();
                    break;

                // 0x00455E59
                case widx::demolish_industry:
                {
                    bool success = GameCommands::do_48(GameCommands::Flags::apply, self->number);
                    if (!success)
                        break;

                    Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
                    break;
                }
            }
        }

        static void initViewport(Window* self);

        // 0x00455F1A
        static void onResize(Window* self)
        {
            self->setSize(minWindowSize, maxWindowSize);

            if (self->viewports[0] != nullptr)
            {
                uint16_t newWidth = self->width - 30;
                if (!isEditorMode() && !isSandboxMode())
                    newWidth += 22;

                uint16_t newHeight = self->height - 59;

                auto& viewport = self->viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->view_width = newWidth << viewport->zoom;
                    viewport->view_height = newHeight << viewport->zoom;
                    self->savedView.clear();
                }
            }

            initViewport(self);
        }

        // 0x00456C36
        static void initViewport(Window* self)
        {
            if (self->currentTab != Common::widx::tab_industry - Common::widx::tab_industry)
                return;

            self->callPrepareDraw();

            // Figure out the industry's position on the map.
            auto industry = IndustryManager::get(IndustryId(self->number));
            int16_t tileZ = Map::TileManager::getHeight({ industry->x, industry->y }).landHeight;

            // Compute views.
            SavedView view = {
                industry->x,
                industry->y,
                ZoomLevel::quarter,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                tileZ,
            };
            // view.flags |= (1 << 14);

            uint16_t flags = 0;
            if (self->viewports[0] != nullptr)
            {
                if (self->savedView == view)
                    return;

                flags = self->viewports[0]->flags;
                self->viewportRemove(0);
                ViewportManager::collectGarbage();
            }
            else
            {
                if ((Config::get().flags & Config::Flags::gridlinesOnLandscape) != 0)
                    flags |= ViewportFlags::gridlines_on_landscape;
            }

            self->savedView = view;

            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = Map::Pos3({ industry->x, industry->y, tileZ });
                auto origin = Ui::Point(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = Ui::Size(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(self, 0, origin, size, self->savedView.zoomLevel, tile);
                self->invalidate();
                self->flags |= WindowFlags::viewportNoScrolling;
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
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
            events.viewportRotate = initViewport;
        }
    }

    // 0x00456D2D
    Window* open(IndustryId industryId)
    {
        auto window = WindowManager::bringToFront(WindowType::industry, enumValue(industryId));
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
                Input::toolCancel();

            window = WindowManager::bringToFront(WindowType::industry, enumValue(industryId));
        }

        if (window == nullptr)
        {
            // 0x00456DBC start
            const uint32_t newFlags = WindowFlags::flag_8 | WindowFlags::resizable;
            window = WindowManager::createWindow(WindowType::industry, Industry::windowSize, newFlags, &Industry::events);
            window->number = enumValue(industryId);
            window->minWidth = 192;
            window->minHeight = 137;
            window->maxWidth = 600;
            window->maxHeight = 440;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            if (skin != nullptr)
            {
                window->setColour(WindowColour::primary, skin->colour_0B);
                window->setColour(WindowColour::secondary, skin->colour_0C);
            }
            // 0x00456DBC end

            window->savedView.clear();
        }

        // TODO: only needs to be called once.
        Common::initEvents();

        window->currentTab = Common::widx::tab_industry - Common::widx::tab_industry;
        window->invalidate();

        window->widgets = Industry::widgets;
        window->enabledWidgets = Industry::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Industry::events;
        window->activatedWidgets = 0;

        Common::setDisabledWidgets(window);

        window->initScrollWidgets();
        Industry::initViewport(window);

        return window;
    }

    namespace Production
    {

        static const Ui::Size minWindowSize = { 299, 282 };

        static const Ui::Size maxWindowSize = { 299, 337 };

        static WindowEventList events;

        // 0x00455FD9
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            Common::repositionTabs(self);
        }

        // 0x0045654F
        static void onResize(Window* self)
        {
            {
                self->setSize(minWindowSize, maxWindowSize);
            }
        }

        static void initEvents()
        {
            events.draw = Common::draw;
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
        }
    }

    namespace Production2
    {
        static const Ui::Size minWindowSize = { 299, 282 };

        static const Ui::Size maxWindowSize = { 299, 337 };

        static Widget widgets[] = {
            commonWidgets(222, 136, StringIds::title_industry_monthly_production),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0045626F
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            Common::repositionTabs(self);
        }

        // 0x004565FF
        static void onResize(Window* self)
        {
            {
                self->setSize(minWindowSize, maxWindowSize);
            }
        }

        static void initEvents()
        {
            events.draw = Common::draw;
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
        }
    }

    namespace Transported
    {
        static const Ui::Size windowSize = { 300, 127 };

        static Widget widgets[] = {
            commonWidgets(300, 126, StringIds::title_statistics),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x00456665
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            Common::repositionTabs(self);
        }

        // 0x00456705
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto industry = IndustryManager::get(IndustryId(self->number));
            const auto* industryObj = industry->getObject();
            int16_t xPos = self->x + 3;
            int16_t yPos = self->y + 45;
            Ui::Point origin = { xPos, yPos };

            // Draw Last Months received cargo stats
            if (industry->canReceiveCargo())
            {
                origin.x += 4;
                origin.y += 10;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::received_cargo);

                auto cargoNumber = 0;
                for (const auto& receivedCargoType : industryObj->required_cargo_type)
                {
                    if (receivedCargoType != 0xFF)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(receivedCargoType);
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

                        origin.y = Gfx::drawString_495224(*context, origin.x, origin.y, 290, Colour::black, StringIds::black_stringid, &args);
                    }
                    cargoNumber++;
                }
                origin.y += 4;
                origin.x -= 4;
            }

            // Draw Last Months produced cargo stats
            if (industry->canProduceCargo())
            {
                Gfx::drawString_494B3F(*context, origin.x, origin.y, Colour::black, StringIds::produced_cargo);
                origin.y += 10;
                origin.x += 4;

                auto cargoNumber = 0;
                for (const auto& producedCargoType : industryObj->produced_cargo_type)
                {
                    if (producedCargoType != 0xFF)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(producedCargoType);
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

                        origin.y = Gfx::drawString_495224(*context, origin.x, origin.y, 290, Colour::black, StringIds::transported_cargo, &args);
                    }
                    cargoNumber++;
                }
            }
        }

        // 0x004569C2
        static void onResize(Window* self)
        {
            {
                self->setSize(windowSize, windowSize);
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t* enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { Industry::widgets, widx::tab_industry, &Industry::events, &Industry::enabledWidgets },
            { Production2::widgets, widx::tab_production, &Production::events, &Common::enabledWidgets },
            { Production2::widgets, widx::tab_production_2, &Production2::events, &Common::enabledWidgets },
            { Transported::widgets, widx::tab_transported, &Transported::events, &Common::enabledWidgets }
        };

        static void setDisabledWidgets(Window* self)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(IndustryManager::get(IndustryId(self->number))->object_id);
            auto disabledWidgets = 0;

            if (industryObj->produced_cargo_type[0] == 0xFF)
                disabledWidgets |= (1 << Common::widx::tab_production);

            if (industryObj->produced_cargo_type[1] == 0xFF)
                disabledWidgets |= (1 << Common::widx::tab_production_2);

            self->disabledWidgets = disabledWidgets;
        }

        // 0x00456079
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            // Draw Units of Cargo sub title
            const auto industry = IndustryManager::get(IndustryId(self->number));
            const auto industryObj = ObjectManager::get<IndustryObject>(industry->object_id);
            const auto cargoObj = ObjectManager::get<CargoObject>(industryObj->produced_cargo_type[0]);

            {
                auto args = FormatArguments();
                args.push(cargoObj->units_and_cargo_name);

                int16_t x = self->x + 2;
                int16_t y = self->y - 24 + 68;

                Gfx::drawString_494B3F(*context, x, y, Colour::black, StringIds::production_graph_label, &args);
            }

            // Draw Y label and grid lines.
            const uint16_t graphBottom = self->y + self->height - 7;
            int32_t yTick = 0;
            for (int16_t yPos = graphBottom; yPos >= self->y + 68; yPos -= 20)
            {
                auto args = FormatArguments();
                args.push(yTick);

                Gfx::drawRect(*context, self->x + 41, yPos, 239, 1, Colours::getShade(self->getColour(WindowColour::secondary).c(), 4));

                Gfx::drawString_494C78(*context, self->x + 39, yPos - 6, Colour::black, StringIds::population_graph_people, &args);

                yTick += 1000;
            }

            MonthId month = getCurrentMonth();
            int16_t year = getCurrentYear();
            int8_t yearSkip = 0;
            // This is either 0 or 1 depending on selected tab
            // used to select the correct history
            const uint8_t productionTabWidx = self->currentTab + widx::tab_industry;
            const uint8_t productionNum = productionTabWidx - widx::tab_production;
            for (uint8_t i = industry->historySize[productionNum] - 1; i > 0; i--)
            {
                const uint16_t xPos = self->x + 41 + i;
                const uint16_t yPos = self->y + 56;

                // Draw horizontal year and vertical grid lines.
                if (month == MonthId::january)
                {
                    if (yearSkip == 0)
                    {
                        auto args = FormatArguments();
                        args.push(year);

                        Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::population_graph_year, &args);
                    }

                    Gfx::drawRect(*context, xPos, yPos + 11, 1, self->height - 74, Colours::getShade(self->getColour(WindowColour::secondary).c(), 4));
                }

                const auto history = productionTabWidx == widx::tab_production ? industry->history_1 : industry->history_2;
                // Draw production graph
                const uint16_t yPos1 = graphBottom - history[i];
                const uint16_t yPos2 = graphBottom - history[i + 1];

                // Do not draw current segment yet; it may be zeroed.
                if (i < industry->historySize[productionNum] - 1)
                {
                    if (yPos1 <= graphBottom)
                    {
                        if (yPos2 <= graphBottom)
                        {
                            Gfx::drawLine(*context, xPos, yPos1, xPos + 1, yPos2, Colours::getShade(self->getColour(WindowColour::secondary).c(), 7));
                        }
                    }
                }

                if (month == MonthId::january)
                {
                    month = MonthId::december;
                    year--;

                    yearSkip++;
                    if (yearSkip >= 3)
                        yearSkip = 0;
                }
                else
                {
                    month = MonthId(static_cast<int8_t>(month) - 1);
                }
            }
        }

        // 0x004565B5, 0x00456505
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameIndustryPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_industry:
                case Common::widx::tab_production:
                case Common::widx::tab_production_2:
                case Common::widx::tab_transported:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->currentTab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab.
            self->activatedWidgets &= ~((1ULL << widx::tab_industry) | (1ULL << widx::tab_production) | (1ULL << widx::tab_production_2) | (1ULL << widx::tab_transported));
            widx widgetIndex = tabInformationByTabOffset[self->currentTab].widgetIndex;
            self->activatedWidgets |= (1ULL << widgetIndex);

            // Put industry name in place.
            auto industry = IndustryManager::get(IndustryId(self->number));
            auto args = FormatArguments();
            args.push(industry->name);
            args.push(industry->town);

            // Resize common widgets.
            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;
        }

        // 0x00455CBC
        static void textInput(Window* self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget != Common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            GameCommands::setErrorTitle(StringIds::error_cant_rename_industry);

            uint32_t* buffer = (uint32_t*)input;
            GameCommands::do_79(IndustryId(self->number), 1, buffer[0], buffer[1], buffer[2]);
            GameCommands::do_79(IndustryId(0), 2, buffer[3], buffer[4], buffer[5]);
            GameCommands::do_79(IndustryId(0), 0, buffer[6], buffer[7], buffer[8]);
        }

        static void update(Window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::industry, self->number);
        }

        // 0x00455D81
        static void renameIndustryPrompt(Window* self, WidgetIndex_t widgetIndex)
        {
            auto industry = IndustryManager::get(IndustryId(self->number));
            if (!isEditorMode() && !isSandboxMode())
            {
                if ((industry->flags & IndustryFlags::flag_04) == 0)
                    return;
                if (!CompanyManager::isPlayerCompany(industry->owner))
                    return;
            }

            auto args = FormatArguments();
            args.push<int64_t>(0);
            args.push(industry->name);
            args.push(industry->town);

            TextInput::openTextInput(self, StringIds::title_industry_name, StringIds::prompt_enter_new_industry_name, industry->name, widgetIndex, &industry->town);
        }

        // 0x00456A5E, 0x00456A64
        static void repositionTabs(Window* self)
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
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_industry;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->var_85C = -1;

            self->viewportRemove(0);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_industry];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;

            Common::setDisabledWidgets(self);

            self->invalidate();

            self->setSize(Industry::windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        static void drawProductionTab(Window* self, Gfx::Context* context, uint8_t productionTabNumber)
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

            auto industry = IndustryManager::get(IndustryId(self->number));
            auto industryObj = ObjectManager::get<IndustryObject>(industry->object_id);
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            static const uint32_t productionTabIds[] = {
                widx::tab_production,
                widx::tab_production_2,
            };

            auto tab = productionTabIds[productionTabNumber];

            uint32_t imageId = 0xFFFFFFFF;
            auto widget = self->widgets[tab];

            if (industryObj->produced_cargo_type[productionTabNumber] != 0xFF)
            {
                imageId = Gfx::recolour(skin->img, self->getColour(WindowColour::secondary).c());

                if (self->currentTab == tab - widx::tab_industry)
                    imageId += productionTabImageIds[(self->frame_no / 4) % std::size(productionTabImageIds)];
                else
                    imageId += productionTabImageIds[0];

                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                Gfx::drawImage(context, xPos, yPos, imageId);

                auto caroObj = ObjectManager::get<CargoObject>(industryObj->produced_cargo_type[productionTabNumber]);
                Gfx::drawImage(context, xPos + 18, yPos + 14, caroObj->unit_inline_sprite);

                Widget::drawTab(self, context, -2, tab);
            }
        }

        // 0x00456A98
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Industry tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_industries;
                Widget::drawTab(self, context, imageId, widx::tab_industry);
            }

            // Production Tab
            {
                drawProductionTab(self, context, 0);
            }

            // 2nd Production Tab
            {
                drawProductionTab(self, context, 1);
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
                if (self->currentTab == widx::tab_transported - widx::tab_industry)
                    imageId += transportedTabImageIds[(self->frame_no / 4) % std::size(transportedTabImageIds)];
                else
                    imageId += transportedTabImageIds[0];
                Widget::drawTab(self, context, imageId, widx::tab_transported);
            }
        }

        static void initEvents()
        {
            Industry::initEvents();
            Production::initEvents();
            Production2::initEvents();
            Transported::initEvents();
        }
    }
}
