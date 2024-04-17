#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Town/RemoveTown.h"
#include "GameCommands/Town/RenameTown.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Ui::Windows::Town
{
    static constexpr Ui::Size kWindowSize = { 223, 161 };

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
        static void prepareDraw(Window& self);
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input);
        static void update(Window& self);
        static void renameTownPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window* self, Gfx::RenderTarget* rt);
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

        static constexpr Widget widgets[] = {
            commonWidgets(223, 161, StringIds::title_town),
            makeWidget({ 3, 44 }, { 195, 104 }, WidgetType::viewport, WindowColour::secondary, Widget::kContentUnk),
            makeWidget({ 3, 139 }, { 195, 21 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::viewportCentreButton, WindowColour::secondary, Widget::kContentNull, StringIds::move_main_view_to_show_this),
            makeWidget({ 198, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::town_expand, StringIds::expand_this_town),
            makeWidget({ 198, 68 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rubbish_bin, StringIds::demolish_this_town),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << centre_on_viewport) | (1 << expand_town) | (1 << demolish_town);

        // 0x00498EAF
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::viewport].right = self.width - 26;
            self.widgets[widx::viewport].bottom = self.height - 14;

            self.widgets[widx::status_bar].top = self.height - 12;
            self.widgets[widx::status_bar].bottom = self.height - 3;
            self.widgets[widx::status_bar].right = self.width - 14;

            self.widgets[widx::expand_town].right = self.width - 2;
            self.widgets[widx::expand_town].left = self.width - 25;

            self.widgets[widx::demolish_town].right = self.width - 2;
            self.widgets[widx::demolish_town].left = self.width - 25;

            if (isEditorMode() || isSandboxMode())
            {
                self.widgets[widx::expand_town].type = WidgetType::buttonWithImage;
                self.widgets[widx::demolish_town].type = WidgetType::buttonWithImage;
            }
            else
            {
                self.widgets[widx::expand_town].type = WidgetType::none;
                self.widgets[widx::demolish_town].type = WidgetType::none;
                self.widgets[widx::viewport].right += 22;
            }

            self.widgets[widx::centre_on_viewport].right = self.widgets[widx::viewport].right - 1;
            self.widgets[widx::centre_on_viewport].bottom = self.widgets[widx::viewport].bottom - 1;
            self.widgets[widx::centre_on_viewport].left = self.widgets[widx::viewport].right - 24;
            self.widgets[widx::centre_on_viewport].top = self.widgets[widx::viewport].bottom - 24;

            Widget::leftAlignTabs(self, Common::widx::tab_town, Common::widx::tab_company_ratings);
        }

        // 0x00498FFE
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto town = TownManager::get(TownId(self.number));

            FormatArguments args{};
            args.push(town->getTownSizeString());
            args.push(town->population);

            const auto& widget = self.widgets[widx::status_bar];
            const auto width = widget.width() - 1;
            auto point = Point(self.x + widget.left - 1, self.y + widget.top - 1);
            drawingCtx.drawStringLeftClipped(*rt, point, width, Colour::black, StringIds::status_town_population, &args);
        }

        // 0x00499079
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameTownPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_town:
                case Common::widx::tab_population:
                case Common::widx::tab_company_ratings:
                    Common::switchTab(&self, widgetIndex);
                    break;

                // 0x0049932D
                case widx::centre_on_viewport:
                    self.viewportCentreMain();
                    break;

                // 0x004990B9
                case widx::expand_town:
                {
                    auto town = TownManager::get(TownId(self.number));

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
                    int16_t tileZ = World::TileManager::getHeight({ town->x, town->y }).landHeight;
                    Audio::playSound(Audio::SoundId::construct, World::Pos3(town->x + 16, town->y + 16, tileZ));
                    break;
                }

                // 0x0049916A
                case widx::demolish_town:
                {
                    GameCommands::setErrorTitle(StringIds::cant_remove_town);

                    GameCommands::TownRemovalArgs args{};
                    args.townId = static_cast<TownId>(self.number);

                    if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
                    {
                        Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
                    }

                    break;
                }
            }
        }

        static void initViewport(Window& self);

        // 0x004993A5
        static void onResize(Window& self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self.setSize(Ui::Size(192, 161), Ui::Size(600, 440));

            if (self.viewports[0] != nullptr)
            {
                uint16_t newWidth = self.width - 30;
                if (!isEditorMode() && !isSandboxMode())
                    newWidth += 22;

                uint16_t newHeight = self.height - 59;

                auto& viewport = self.viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->viewWidth = newWidth << viewport->zoom;
                    viewport->viewHeight = newHeight << viewport->zoom;
                    self.savedView.clear();
                }
            }

            initViewport(self);
        }

        // 0x00499A87
        static void initViewport(Window& self)
        {
            if (self.currentTab != 0)
                return;

            self.callPrepareDraw();

            // Figure out the town's position on the map.
            auto town = TownManager::get(TownId(self.number));
            int16_t tileZ = World::TileManager::getHeight({ town->x, town->y }).landHeight;

            // Compute views.
            SavedView view = {
                town->x,
                town->y,
                ZoomLevel::quarter,
                static_cast<int8_t>(self.viewports[0]->getRotation()),
                tileZ,
            };

            ViewportFlags flags = ViewportFlags::none;
            if (self.viewports[0] != nullptr)
            {
                if (self.savedView == view)
                    return;

                flags = self.viewports[0]->flags;
                self.viewportRemove(0);
                ViewportManager::collectGarbage();
            }
            else
            {
                if (Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
                    flags |= ViewportFlags::gridlines_on_landscape;
            }

            self.savedView = view;

            // 0x00499B39 start
            if (self.viewports[0] == nullptr)
            {
                auto widget = &self.widgets[widx::viewport];
                auto tile = World::Pos3({ town->x, town->y, tileZ });
                auto origin = Ui::Point(widget->left + self.x + 1, widget->top + self.y + 1);
                auto size = Ui::Size(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(&self, 0, origin, size, self.savedView.zoomLevel, tile);
                self.invalidate();
                self.flags |= WindowFlags::viewportNoScrolling;
            }
            // 0x00499B39 end

            if (self.viewports[0] != nullptr)
            {
                self.viewports[0]->flags = flags;
                self.invalidate();
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::update,
            .textInput = Common::textInput,
            .viewportRotate = initViewport,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x00499B7E
    Window* open(uint16_t townId)
    {
        auto window = WindowManager::bringToFront(WindowType::town, townId);
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
                ToolManager::toolCancel();

            window = WindowManager::bringToFront(WindowType::town, townId);
        }

        if (window == nullptr)
        {
            // 0x00499C0D start
            const WindowFlags newFlags = WindowFlags::flag_8 | WindowFlags::resizable;
            window = WindowManager::createWindow(WindowType::town, kWindowSize, newFlags, Town::getEvents());
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

        window->currentTab = 0;
        window->invalidate();

        window->setWidgets(Town::widgets);
        window->enabledWidgets = Town::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Town::getEvents();
        window->activatedWidgets = 0;
        window->disabledWidgets = 0;
        window->initScrollWidgets();
        Town::initViewport(*window);

        return window;
    }

    namespace Population
    {
        static constexpr Widget widgets[] = {
            commonWidgets(223, 161, StringIds::title_town_population),
            widgetEnd(),
        };

        // 0x00499469
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);
        }

        // 0x004994F9
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(self.x, self.y + 44, self.width, self.height - 44));
            if (!clipped)
                return;

            auto town = TownManager::get(TownId(self.number));

            // Draw Y label and grid lines.
            int32_t yTick = town->historyMinPopulation;
            for (int16_t yPos = self.height - 57; yPos >= 14; yPos -= 20)
            {
                FormatArguments args{};
                args.push(yTick);

                const uint16_t xPos = 39;
                drawingCtx.drawRect(*clipped, xPos, yPos, 241, 1, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4), Gfx::RectFlags::none);

                auto point = Point(xPos, yPos - 6);
                drawingCtx.drawStringRight(*clipped, point, Colour::black, StringIds::population_graph_people, &args);

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
                        FormatArguments args{};
                        args.push(year);

                        auto point = Point(xPos, yPos);
                        drawingCtx.drawStringCentred(*clipped, point, Colour::black, StringIds::population_graph_year, &args);
                    }

                    drawingCtx.drawRect(*clipped, xPos, 11, 1, self.height - 66, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4), Gfx::RectFlags::none);
                }

                // Draw population graph
                uint16_t yPos1 = -town->history[i] + (self.height - 57);
                uint16_t yPos2 = -town->history[i + 1] + (self.height - 57);

                // Do not draw current segment yet; it may be zeroed.
                if (i < town->historySize - 1)
                    drawingCtx.drawLine(*clipped, Ui::Point(xPos, yPos1), Ui::Point(xPos + 1, yPos2), Colours::getShade(self.getColour(WindowColour::secondary).c(), 7));

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
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameTownPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_town:
                case Common::widx::tab_population:
                case Common::widx::tab_company_ratings:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x004996F6
        static void onResize(Window& self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self.setSize(Ui::Size(299, 172), Ui::Size(299, 327));
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::update,
            .textInput = Common::textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace CompanyRatings
    {
        static constexpr Widget widgets[] = {
            commonWidgets(340, 208, StringIds::title_town_local_authority),
            widgetEnd(),
        };

        // 0x00499761
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);
        }

        // 0x004997F1
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto point = Point(self.x + 4, self.y + 46);
            drawingCtx.drawStringLeft(*rt, point, Colour::black, StringIds::local_authority_ratings_transport_companies);

            point.x += 4;
            point.y += 14;
            auto town = TownManager::get(TownId(self.number));
            for (uint8_t i = 0; i < std::size(town->companyRatings); i++)
            {
                if ((town->companiesWithRating & (1 << i)) == 0)
                    continue;

                int16_t rating = (std::clamp<int16_t>(town->companyRatings[i], -1000, 1000) + 1000) / 20;
                StringId rank{};
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

                FormatArguments args{};
                args.push(CompanyManager::get(CompanyId(i))->name);
                args.push<int16_t>(0);
                args.push(rating);
                args.push(rank);

                drawingCtx.drawStringLeftClipped(*rt, point, self.width - 12, Colour::black, StringIds::town_rating_company_percentage_rank, &args);

                point.y += 10;
            }
        }

        // 0x004998E7
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameTownPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_town:
                case Common::widx::tab_population:
                case Common::widx::tab_company_ratings:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x00499936
        static void onResize(Window& self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self.setSize(Ui::Size(340, 208), Ui::Size(340, 208));
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::update,
            .textInput = Common::textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t* enabledWidgets;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { Town::widgets,           widx::tab_town,            Town::getEvents(),           &Town::enabledWidgets },
            { Population::widgets,     widx::tab_population,      Population::getEvents(),     &Common::enabledWidgets },
            { CompanyRatings::widgets, widx::tab_company_ratings, CompanyRatings::getEvents(), &Common::enabledWidgets }
        };
        // clang-format on

        static void prepareDraw(Window& self)
        {
            // Activate the current tab.
            self.activatedWidgets &= ~((1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings));
            widx widgetIndex = tabInformationByTabOffset[self.currentTab].widgetIndex;
            self.activatedWidgets |= (1ULL << widgetIndex);

            // Put town name in place.
            FormatArguments args{};
            args.push(TownManager::get(TownId(self.number))->name);

            // Resize common widgets.
            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
        }

        // 0x00499287
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget != Common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            GameCommands::setErrorTitle(StringIds::error_cant_rename_town);

            GameCommands::RenameTownArgs args{};

            args.townId = TownId(self.number);
            args.nameBufferIndex = 1;
            std::memcpy(args.buffer, input, 36);

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.nameBufferIndex = 2;

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.nameBufferIndex = 0;

            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        static void update(Window& self)
        {
            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::station, self.number);
        }

        static void renameTownPrompt(Window* self, WidgetIndex_t widgetIndex)
        {
            auto town = TownManager::get(TownId(self->number));

            FormatArguments args{};
            args.skip(4);
            args.push(town->name);
            args.push(town->name);

            TextInput::openTextInput(self, StringIds::title_town_name, StringIds::prompt_type_new_town_name, town->name, widgetIndex, &args);
        }

        // 0x004991BC
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self->type, self->number))
                ToolManager::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_town;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->var_85C = -1;

            self->viewportRemove(0);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_town];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = &tabInfo.events;
            self->activatedWidgets = 0;
            self->setWidgets(tabInfo.widgets);
            self->disabledWidgets = 0;

            self->invalidate();

            self->setSize(kWindowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x004999E1
        static void drawTabs(Window* self, Gfx::RenderTarget* rt)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Town tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_towns;
                Widget::drawTab(self, rt, imageId, widx::tab_town);
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
                    imageId += populationTabImageIds[(self->frameNo / 4) % std::size(populationTabImageIds)];
                else
                    imageId += populationTabImageIds[0];

                Widget::drawTab(self, rt, imageId, widx::tab_population);
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
                    imageId += ratingsTabImageIds[(self->frameNo / 4) % std::size(ratingsTabImageIds)];
                else
                    imageId += ratingsTabImageIds[0];

                Widget::drawTab(self, rt, imageId, widx::tab_company_ratings);
            }
        }
    }
}
