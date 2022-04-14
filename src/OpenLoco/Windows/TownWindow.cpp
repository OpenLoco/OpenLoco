#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Date.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Map.hpp"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../TownManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Ui::Windows::Town
{
    static const Ui::Size windowSize = { 223, 161 };

    static loco_global<uint16_t[10], 0x0112C826> commonFormatArgs;

    namespace Common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_town,
            tab_population,
            tab_company_ratings,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 120 }, WidgetType::panel, WindowColour::secondary),                                                                      \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town),                                    \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_population_graph),                       \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town_ratings_each_company)

        // Defined at the bottom of this file.
        static void prepareDraw(Window* self);
        static void textInput(Window* self, WidgetIndex_t callingWidget, const char* input);
        static void update(Window* self);
        static void renameTownPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void repositionTabs(Window* self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void initEvents();
    }

    namespace Town
    {
        enum widx
        {
            viewport = 7,
            status_bar,
            centre_on_viewport,
            expand_town,
            demolish_town,
        };

        static Widget widgets[] = {
            commonWidgets(223, 161, StringIds::title_town),
            makeWidget({ 3, 44 }, { 195, 104 }, WidgetType::viewport, WindowColour::secondary, 0xFFFFFFFE),
            makeWidget({ 3, 139 }, { 195, 21 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::null, StringIds::move_main_view_to_show_this),
            makeWidget({ 198, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::town_expand, StringIds::expand_this_town),
            makeWidget({ 198, 68 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rubbish_bin, StringIds::demolish_this_town),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << centre_on_viewport) | (1 << expand_town) | (1 << demolish_town);

        static WindowEventList events;

        // 0x00498EAF
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::expand_town].right = self->width - 2;
            self->widgets[widx::expand_town].left = self->width - 25;

            self->widgets[widx::demolish_town].right = self->width - 2;
            self->widgets[widx::demolish_town].left = self->width - 25;

            if (isEditorMode() || isSandboxMode())
            {
                self->widgets[widx::expand_town].type = WidgetType::buttonWithImage;
                self->widgets[widx::demolish_town].type = WidgetType::buttonWithImage;
            }
            else
            {
                self->widgets[widx::expand_town].type = WidgetType::none;
                self->widgets[widx::demolish_town].type = WidgetType::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            Common::repositionTabs(self);
        }

        // 0x00498FFE
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
            self->drawViewports(context);
            Widget::drawViewportCentreButton(context, self, widx::centre_on_viewport);

            auto town = TownManager::get(TownId(self->number));

            auto args = FormatArguments();
            args.push(town->getTownSizeString());
            args.push(town->population);

            const auto& widget = self->widgets[widx::status_bar];
            const auto x = self->x + widget.left - 1;
            const auto y = self->y + widget.top - 1;
            const auto width = widget.width() - 1;
            Gfx::drawString_494BBF(*context, x, y, width, Colour::black, StringIds::status_town_population, &args);
        }

        // 0x00499079
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameTownPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_town:
                case Common::widx::tab_population:
                case Common::widx::tab_company_ratings:
                    Common::switchTab(self, widgetIndex);
                    break;

                // 0x0049932D
                case widx::centre_on_viewport:
                    self->viewportCentreMain();
                    break;

                // 0x004990B9
                case widx::expand_town:
                {
                    auto town = TownManager::get(TownId(self->number));

                    const uint32_t ebx = (town->numBuildings >> 3) + 5;
                    const int16_t currentYear = getCurrentYear();
                    int16_t tempYear = currentYear - 51;
                    setCurrentYear(tempYear);

                    for (uint8_t i = 8; i > 0; i--)
                    {
                        for (uint32_t j = ebx; j > 0; j--)
                        {
                            town->grow(0xFF);
                            town->recalculateSize();
                        }

                        tempYear += 7;
                        setCurrentYear(tempYear);
                    }

                    setCurrentYear(currentYear);

                    // Set new history entry.
                    uint8_t historyEntry = std::min<uint8_t>(town->population / 50, 255);
                    town->history[town->historySize - 1] = historyEntry;

                    // Play construction sound at the town centre.
                    int16_t tileZ = Map::TileManager::getHeight({ town->x, town->y }).landHeight;
                    Audio::playSound(Audio::SoundId::construct, Map::Pos3(town->x + 16, town->y + 16, tileZ));
                    break;
                }

                // 0x0049916A
                case widx::demolish_town:
                {
                    GameCommands::setErrorTitle(StringIds::cant_remove_town);
                    bool success = GameCommands::do_50(self->number);
                    if (!success)
                        break;

                    Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
                    break;
                }
            }
        }

        static void initViewport(Window* self);

        // 0x004993A5
        static void onResize(Window* self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self->setSize(Ui::Size(192, 161), Ui::Size(600, 440));

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

        // 0x00499A87
        static void initViewport(Window* self)
        {
            if (self->currentTab != 0)
                return;

            self->callPrepareDraw();

            // Figure out the town's position on the map.
            auto town = TownManager::get(TownId(self->number));
            int16_t tileZ = Map::TileManager::getHeight({ town->x, town->y }).landHeight;

            // Compute views.
            SavedView view = {
                town->x,
                town->y,
                ZoomLevel::quarter,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                tileZ,
            };

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

            // 0x00499B39 start
            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = Map::Pos3({ town->x, town->y, tileZ });
                auto origin = Ui::Point(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = Ui::Size(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(self, 0, origin, size, self->savedView.zoomLevel, tile);
                self->invalidate();
                self->flags |= WindowFlags::viewportNoScrolling;
            }
            // 0x00499B39 end

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

    // 0x00499B7E
    Window* open(uint16_t townId)
    {
        auto window = WindowManager::bringToFront(WindowType::town, townId);
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
                Input::toolCancel();

            window = WindowManager::bringToFront(WindowType::town, townId);
        }

        if (window == nullptr)
        {
            // 0x00499C0D start
            const uint32_t newFlags = WindowFlags::flag_8 | WindowFlags::resizable;
            window = WindowManager::createWindow(WindowType::town, windowSize, newFlags, &Town::events);
            window->number = townId;
            window->minWidth = 192;
            window->minHeight = 161;
            window->maxWidth = 600;
            window->maxHeight = 440;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            if (skin != nullptr)
            {
                window->setColour(WindowColour::primary, skin->colour_0B);
                window->setColour(WindowColour::secondary, skin->colour_0C);
            }
            // 0x00499C0D end

            window->savedView.clear();
        }

        // TODO(avgeffen): only needs to be called once.
        Common::initEvents();

        window->currentTab = 0;
        window->invalidate();

        window->widgets = Town::widgets;
        window->enabledWidgets = Town::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Town::events;
        window->activatedWidgets = 0;
        window->disabledWidgets = 0;
        window->initScrollWidgets();
        Town::initViewport(window);

        return window;
    }

    namespace Population
    {
        static Widget widgets[] = {
            commonWidgets(223, 161, StringIds::title_town_population),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x00499469
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);
        }

        // 0x004994F9
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto clipped = Gfx::clipContext(*context, Ui::Rect(self->x, self->y + 44, self->width, self->height - 44));
            if (!clipped)
                return;

            auto town = TownManager::get(TownId(self->number));

            // Draw Y label and grid lines.
            int32_t yTick = town->history_min_population;
            for (int16_t yPos = self->height - 57; yPos >= 14; yPos -= 20)
            {
                auto args = FormatArguments();
                args.push(yTick);

                const uint16_t xPos = 39;
                Gfx::drawRect(*clipped, xPos, yPos, 241, 1, Colours::getShade(self->getColour(WindowColour::secondary).c(), 4));

                Gfx::drawString_494C78(*clipped, xPos, yPos - 6, Colour::black, StringIds::population_graph_people, &args);

                yTick += 1000;
            }

            int8_t month = static_cast<int8_t>(getCurrentMonth());
            int16_t year = getCurrentYear();
            int8_t yearSkip = 0;

            for (uint8_t i = town->historySize - 1; i > 0; i--)
            {
                const uint16_t xPos = 41 + i;
                const uint16_t yPos = 2;

                // Draw horizontal year and vertical grid lines.
                if (month == 0)
                {
                    if (yearSkip == 0)
                    {
                        auto args = FormatArguments();
                        args.push(year);

                        Gfx::drawStringCentred(*clipped, xPos, yPos, Colour::black, StringIds::population_graph_year, &args);
                    }

                    Gfx::drawRect(*clipped, xPos, 11, 1, self->height - 66, Colours::getShade(self->getColour(WindowColour::secondary).c(), 4));
                }

                // Draw population graph
                uint16_t yPos1 = -town->history[i] + (self->height - 57);
                uint16_t yPos2 = -town->history[i + 1] + (self->height - 57);

                // Do not draw current segment yet; it may be zeroed.
                if (i < town->historySize - 1)
                    Gfx::drawLine(*clipped, xPos, yPos1, xPos + 1, yPos2, Colours::getShade(self->getColour(WindowColour::secondary).c(), 7));

                month--;
                if (month < 0)
                {
                    month = 11;
                    year--;

                    yearSkip++;
                    if (yearSkip >= 3)
                        yearSkip = 0;
                }
            }
        }

        // 0x004996AC
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameTownPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_town:
                case Common::widx::tab_population:
                case Common::widx::tab_company_ratings:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x004996F6
        static void onResize(Window* self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self->setSize(Ui::Size(299, 172), Ui::Size(299, 327));
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
        }
    }

    namespace CompanyRatings
    {
        static Widget widgets[] = {
            commonWidgets(340, 208, StringIds::title_town_local_authority),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x00499761
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);
        }

        // 0x004997F1
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            uint16_t xPos = self->x + 4;
            uint16_t yPos = self->y + 46;
            Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::local_authority_ratings_transport_companies);

            xPos += 4;
            yPos += 14;
            auto town = TownManager::get(TownId(self->number));
            for (uint8_t i = 0; i < std::size(town->company_ratings); i++)
            {
                if ((town->companies_with_rating & (1 << i)) == 0)
                    continue;

                int16_t rating = (std::clamp<int16_t>(town->company_ratings[i], -1000, 1000) + 1000) / 20;
                string_id rank{};
                if (rating >= 70)
                    rank = StringIds::town_rating_excellent;
                else if (rating >= 60)
                    rank = StringIds::town_rating_good;
                else if (rating >= 50)
                    rank = StringIds::town_rating_average;
                else if (rating >= 25)
                    rank = StringIds::town_rating_poor;
                else
                    rank = StringIds::town_rating_appalling;

                auto args = FormatArguments();
                args.push(CompanyManager::get(CompanyId(i))->name);
                args.push<int16_t>(0);
                args.push(rating);
                args.push(rank);

                Gfx::drawString_494BBF(*context, xPos, yPos, self->width - 12, Colour::black, StringIds::town_rating_company_percentage_rank, &args);

                yPos += 10;
            }
        }

        // 0x004998E7
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameTownPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_town:
                case Common::widx::tab_population:
                case Common::widx::tab_company_ratings:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x00499936
        static void onResize(Window* self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self->setSize(Ui::Size(340, 208), Ui::Size(340, 208));
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onMouseUp = onMouseUp;
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
            { Town::widgets, widx::tab_town, &Town::events, &Town::enabledWidgets },
            { Population::widgets, widx::tab_population, &Population::events, &Common::enabledWidgets },
            { CompanyRatings::widgets, widx::tab_company_ratings, &CompanyRatings::events, &Common::enabledWidgets }
        };

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
            self->activatedWidgets &= ~((1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings));
            widx widgetIndex = tabInformationByTabOffset[self->currentTab].widgetIndex;
            self->activatedWidgets |= (1ULL << widgetIndex);

            // Put town name in place.
            commonFormatArgs[0] = TownManager::get(TownId(self->number))->name;

            // Resize common widgets.
            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;
        }

        // 0x00499287
        static void textInput(Window* self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget != Common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            GameCommands::setErrorTitle(StringIds::error_cant_rename_town);

            uint32_t* buffer = (uint32_t*)input;
            GameCommands::do_46(self->number, 1, buffer[0], buffer[1], buffer[2]);
            GameCommands::do_46(0, 2, buffer[3], buffer[4], buffer[5]);
            GameCommands::do_46(0, 0, buffer[6], buffer[7], buffer[8]);
        }

        static void update(Window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::station, self->number);
        }

        static void renameTownPrompt(Window* self, WidgetIndex_t widgetIndex)
        {
            auto town = TownManager::get(TownId(self->number));
            commonFormatArgs[4] = town->name;
            commonFormatArgs[8] = town->name;

            TextInput::openTextInput(self, StringIds::title_town_name, StringIds::prompt_type_new_town_name, town->name, widgetIndex, &commonFormatArgs);
        }

        // 0x004999A7, 0x004999AD
        static void repositionTabs(Window* self)
        {
            int16_t xPos = self->widgets[widx::tab_town].left;
            const int16_t tabWidth = self->widgets[widx::tab_town].right - xPos;

            for (uint8_t i = widx::tab_town; i <= widx::tab_company_ratings; i++)
            {
                if (self->isDisabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x004991BC
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_town;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->var_85C = -1;

            self->viewportRemove(0);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_town];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabledWidgets = 0;

            self->invalidate();

            self->setSize(windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x004999E1
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Town tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_towns;
                Widget::drawTab(self, context, imageId, widx::tab_town);
            }

            // Population tab
            {
                static const uint32_t populationTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_population_frame0,
                    InterfaceSkin::ImageIds::tab_population_frame1,
                    InterfaceSkin::ImageIds::tab_population_frame2,
                    InterfaceSkin::ImageIds::tab_population_frame3,
                    InterfaceSkin::ImageIds::tab_population_frame4,
                    InterfaceSkin::ImageIds::tab_population_frame5,
                    InterfaceSkin::ImageIds::tab_population_frame6,
                    InterfaceSkin::ImageIds::tab_population_frame7,
                };

                uint32_t imageId = Gfx::recolour(skin->img, self->getColour(WindowColour::secondary).c());
                if (self->currentTab == widx::tab_population - widx::tab_town)
                    imageId += populationTabImageIds[(self->frame_no / 4) % std::size(populationTabImageIds)];
                else
                    imageId += populationTabImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_population);
            }

            // Company ratings tab
            {
                static const uint32_t ratingsTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_ratings_frame0,
                    InterfaceSkin::ImageIds::tab_ratings_frame1,
                    InterfaceSkin::ImageIds::tab_ratings_frame2,
                    InterfaceSkin::ImageIds::tab_ratings_frame3,
                    InterfaceSkin::ImageIds::tab_ratings_frame4,
                    InterfaceSkin::ImageIds::tab_ratings_frame5,
                    InterfaceSkin::ImageIds::tab_ratings_frame6,
                    InterfaceSkin::ImageIds::tab_ratings_frame7,
                    InterfaceSkin::ImageIds::tab_ratings_frame8,
                    InterfaceSkin::ImageIds::tab_ratings_frame9,
                    InterfaceSkin::ImageIds::tab_ratings_frame10,
                    InterfaceSkin::ImageIds::tab_ratings_frame11,
                    InterfaceSkin::ImageIds::tab_ratings_frame12,
                    InterfaceSkin::ImageIds::tab_ratings_frame13,
                    InterfaceSkin::ImageIds::tab_ratings_frame14,
                    InterfaceSkin::ImageIds::tab_ratings_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_company_ratings - widx::tab_town)
                    imageId += ratingsTabImageIds[(self->frame_no / 4) % std::size(ratingsTabImageIds)];
                else
                    imageId += ratingsTabImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_company_ratings);
            }
        }

        static void initEvents()
        {
            Town::initEvents();
            Population::initEvents();
            CompanyRatings::initEvents();
        }
    }
}
