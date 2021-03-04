#include "News.h"
#include "../../CompanyManager.h"
#include "../../Date.h"
#include "../../Graphics/Colour.h"
#include "../../Graphics/ImageIds.h"
#include "../../IndustryManager.h"
#include "../../Interop/Interop.hpp"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Map/TileManager.h"
#include "../../Message.h"
#include "../../MessageManager.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/VehicleObject.h"
#include "../../Ptr.h"
#include "../../StationManager.h"
#include "../../Things/ThingManager.h"
#include "../../TownManager.h"
#include "../../Vehicles/Vehicle.h"
#include "../../ViewportManager.h"
#include "../../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::NewsWindow
{
    namespace News1
    {
        widget_t widgets[] = {
            commonWidgets(360, 117, widget_type::wt_3),
            widgetEnd(),
        };

        window_event_list events;

        // 0x00429BB7
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                {
                    if (_activeMessageIndex != MessageId::null)
                    {
                        auto news = MessageManager::get(_activeMessageIndex);
                        news->var_C8 = MessageId::null;
                        _activeMessageIndex = MessageId::null;
                    }
                    WindowManager::close(self);
                    break;
                }

                case Common::widx::viewport1Button:
                case Common::widx::viewport2Button:
                {
                    if (_activeMessageIndex != MessageId::null)
                    {
                        auto news = MessageManager::get(_activeMessageIndex);
                        if (widgetIndex == Common::widx::viewport1Button)
                        {
                            if (!(_word_4F8BE4[news->type] & (1 << 2)))
                                break;
                        }
                        else
                        {
                            if (!(_word_4F8BE4[news->type] & (1 << 3)))
                                break;
                        }

                        uint32_t itemType;
                        uint16_t itemId;
                        if (widgetIndex == Common::widx::viewport1Button)
                        {
                            itemType = _byte_4F8B08[news->type].type;
                            itemId = news->item_id_1;
                        }
                        else
                        {
                            itemType = _byte_4F8B09[news->type].type;
                            itemId = news->item_id_2;
                        }

                        switch (itemType)
                        {
                            case newsItemSubTypes::industry:
                                Ui::Windows::Industry::open(itemId);
                                break;

                            case newsItemSubTypes::station:
                                Ui::Windows::Station::open(itemId);
                                break;

                            case newsItemSubTypes::town:
                                Ui::Windows::Town::open(itemId);
                                break;

                            case newsItemSubTypes::vehicle:
                            {
                                auto vehicle = ThingManager::get<Vehicles::VehicleBase>(itemId);

                                Ui::Vehicle::Main::open(vehicle);
                                break;
                            }

                            case newsItemSubTypes::company:
                                Ui::Windows::CompanyWindow::open(itemId);
                                break;

                            case 5:
                            case 6:
                                break;

                            case newsItemSubTypes::vehicleTab:
                                auto vehicleObj = ObjectManager::get<vehicle_object>(itemId);
                                auto window = Ui::BuildVehicle::open(static_cast<uint32_t>(vehicleObj->type), (1 << 31));
                                window->row_hover = itemId;
                                if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
                                {
                                    if (vehicleObj->track_type != 0xFF)
                                    {
                                        uint8_t i = 0;
                                        while (i < _numTrackTypeTabs)
                                        {
                                            if (vehicleObj->track_type == _trackTypesForTab[i])
                                            {
                                                window->current_secondary_tab = i;
                                                break;
                                            }
                                        }
                                    }
                                }

                                auto rowHover = window->row_hover;

                                Ui::BuildVehicle::sub_4B92A5(window);

                                window->row_hover = rowHover;
                                break;
                        }
                    }
                }
            }
        }

        // 0x00429D2C
        static void onUpdate(window* self)
        {
            uint16_t height = _word_525CE0 + 4;

            _word_525CE0 = std::min(height, self->height);

            height = Ui::height() - _word_525CE0 - self->y;
            auto width = (Ui::width() / 2) - (windowSize.width / 2) - self->x;

            if (width != 0 || height != 0)
            {
                self->invalidate();
                self->y += height;
                self->x += width;

                if (self->viewports[0] != nullptr)
                {
                    self->viewports[0]->x += width;
                    self->viewports[0]->y += height;
                }

                if (self->viewports[1] != nullptr)
                {
                    self->viewports[1]->x += width;
                    self->viewports[1]->y += height;
                }

                self->invalidate();
            }
        }

        static SavedView getView(window* self, message* news, uint16_t itemId, uint8_t itemType, bool* selectable)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)-1;
            view.thingId = 0xFFFF;
            switch (itemType)
            {
                case newsItemSubTypes::industry:
                {
                    auto industry = IndustryManager::get(itemId);

                    view.mapX = industry->x;
                    view.mapY = industry->y;
                    view.surfaceZ = TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.rotation = gCurrentRotation;
                    view.zoomLevel = ZoomLevel::half;
                    *selectable = true;
                    break;
                }

                case newsItemSubTypes::station:
                {
                    auto station = StationManager::get(itemId);

                    view.mapX = station->x;
                    view.mapY = station->y;
                    view.surfaceZ = station->z;
                    view.rotation = gCurrentRotation;
                    view.zoomLevel = ZoomLevel::full;
                    *selectable = true;
                    break;
                }

                case newsItemSubTypes::town:
                {
                    auto town = TownManager::get(itemId);

                    view.mapX = town->x;
                    view.mapY = town->y;
                    view.surfaceZ = TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.rotation = gCurrentRotation;
                    view.zoomLevel = ZoomLevel::half;
                    *selectable = true;
                    break;
                }

                case newsItemSubTypes::vehicle:
                {
                    Vehicles::Vehicle train(itemId);
                    if (train.head->tile_x == -1)
                        break;

                    view.thingId = train.veh2->id;

                    if (!train.cars.empty())
                    {
                        view.thingId = train.cars.firstCar.body->id;
                    }

                    view.flags = (1 << 15);
                    view.zoomLevel = ZoomLevel::full;
                    view.rotation = gCurrentRotation;
                    *selectable = true;
                    break;
                }

                case newsItemSubTypes::company:
                    view.zoomLevel = (ZoomLevel)-2;
                    self->invalidate();
                    *selectable = true;
                    break;

                case 5:
                    view.mapX = news->item_id_1; // possible union?
                    view.mapY = news->item_id_2;
                    view.surfaceZ = TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.zoomLevel = ZoomLevel::full;
                    view.rotation = gCurrentRotation;
                    *selectable = true;
                    break;

                case 6:
                    break;

                case newsItemSubTypes::vehicleTab:
                    view.zoomLevel = (ZoomLevel)-3;
                    self->invalidate();
                    *selectable = true;
                    break;
            }
            return view;
        }

        // 0x00429209
        void initViewport(window* self)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)-1;
            view.thingId = 0xFFFF;
            auto news = MessageManager::get(_activeMessageIndex);

            bool selectable = false;

            if (_activeMessageIndex != 0xFFFF)
            {
                if (_word_4F8BE4[news->type] & (1 << 2))
                {
                    auto itemType = _byte_4F8B08[news->type].type;

                    if (news->item_id_1 != 0xFFFF)
                    {
                        view = getView(self, news, news->item_id_1, itemType, &selectable);
                    }
                }
            }

            self->widgets[Common::widx::viewport1].type = widget_type::none;
            self->widgets[Common::widx::viewport1Button].type = widget_type::none;

            if (!view.isEmpty())
            {
                self->widgets[Common::widx::viewport1].type = widget_type::viewport;
            }

            if (selectable)
            {
                self->widgets[Common::widx::viewport1Button].type = widget_type::wt_9;
            }

            uint32_t ecx = view.surfaceZ << 16 | view.rotation << 8 | (uint8_t)view.zoomLevel;
            uint32_t edx = view.mapY << 16 | view.mapX | 1 << 30;

            if (!view.isEmpty() && view.isThingView())
            {
                ecx = view.rotation << 8 | (uint8_t)view.zoomLevel;
                edx = view.thingId | view.flags << 16;
            }

            if (_dword_525CD0 != ecx || _dword_525CD4 != edx)
            {
                _dword_525CD0 = ecx;
                _dword_525CD4 = edx;
                auto viewport = self->viewports[0];
                if (viewport != nullptr)
                {
                    viewport = nullptr;
                    self->invalidate();
                }

                self->widgets[Common::widx::viewport1].left = 6;
                self->widgets[Common::widx::viewport1].right = 353;
                self->widgets[Common::widx::viewport1Button].left = 4;
                self->widgets[Common::widx::viewport1Button].right = 355;

                if (_word_4F8BE4[news->type] & (1 << 3))
                {
                    self->widgets[Common::widx::viewport1].left = 6;
                    self->widgets[Common::widx::viewport1].right = 173;
                    self->widgets[Common::widx::viewport1Button].left = 4;
                    self->widgets[Common::widx::viewport1Button].right = 175;
                }

                if (edx != 0xFFFFFFFF)
                {
                    int16_t x = self->widgets[Common::widx::viewport1].left + 1 + self->x;
                    int16_t y = self->widgets[Common::widx::viewport1].top + 1 + self->y;
                    Gfx::point_t origin = { x, y };

                    uint16_t viewportWidth = self->widgets[Common::widx::viewport1].width();
                    uint16_t viewportHeight = 62;
                    Gfx::ui_size_t viewportSize = { viewportWidth, viewportHeight };

                    if (_word_4F8BE4[news->type] & (1 << 1))
                    {
                        x = self->widgets[Common::widx::viewport1].left + self->x;
                        y = self->widgets[Common::widx::viewport1].top + self->y;
                        origin = { x, y };

                        viewportWidth = self->widgets[Common::widx::viewport1].width() + 2;
                        viewportHeight = 64;
                        viewportSize = { viewportWidth, viewportHeight };
                    }

                    if (view.isThingView())
                    {
                        ViewportManager::create(self, 0, origin, viewportSize, view.zoomLevel, view.thingId);
                    }
                    else
                    {
                        ViewportManager::create(self, 0, origin, viewportSize, view.zoomLevel, view.getPos());
                    }
                    self->invalidate();
                }
            }

            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)-1;
            view.thingId = 0xFFFF;
            selectable = false;

            if (_activeMessageIndex != 0xFFFF)
            {
                if (_word_4F8BE4[news->type] & (1 << 3))
                {
                    auto itemType = _byte_4F8B09[news->type].type;

                    if (news->item_id_2 != 0xFFFF)
                    {
                        view = getView(self, news, news->item_id_2, itemType, &selectable);
                    }
                }
            }

            self->widgets[Common::widx::viewport2].type = widget_type::none;
            self->widgets[Common::widx::viewport2Button].type = widget_type::none;

            if (!view.isEmpty())
            {
                self->widgets[Common::widx::viewport2].type = widget_type::viewport;
            }

            if (selectable)
            {
                self->widgets[Common::widx::viewport2Button].type = widget_type::wt_9;
            }

            ecx = view.surfaceZ << 16 | view.rotation << 8 | (uint8_t)view.zoomLevel;
            edx = view.mapY << 16 | view.mapX | 1 << 30;

            if (!view.isEmpty() && view.isThingView())
            {
                ecx = view.rotation << 8 | (uint8_t)view.zoomLevel;
                edx = view.thingId | view.flags << 16;
            }

            if (_dword_525CD8 != ecx || _dword_525CDC != edx)
            {
                _dword_525CD8 = ecx;
                _dword_525CDC = edx;
                auto viewport = self->viewports[0];
                if (viewport != nullptr)
                {
                    viewport = nullptr;
                    self->invalidate();
                }

                self->widgets[Common::widx::viewport2].left = 186;
                self->widgets[Common::widx::viewport2].right = 353;
                self->widgets[Common::widx::viewport2Button].left = 184;
                self->widgets[Common::widx::viewport2Button].right = 355;

                if (edx != 0xFFFFFFFF)
                {
                    int16_t x = self->widgets[Common::widx::viewport2].left + 1 + self->x;
                    int16_t y = self->widgets[Common::widx::viewport2].top + 1 + self->y;
                    Gfx::point_t origin = { x, y };

                    uint16_t viewportWidth = self->widgets[Common::widx::viewport2].width();
                    uint16_t viewportHeight = 62;
                    Gfx::ui_size_t viewportSize = { viewportWidth, viewportHeight };

                    if (_word_4F8BE4[news->type] & (1 << 1))
                    {
                        x = self->widgets[Common::widx::viewport2].left + self->x;
                        y = self->widgets[Common::widx::viewport2].top + self->y;
                        origin = { x, y };

                        viewportWidth = self->widgets[Common::widx::viewport2].width() + 2;
                        viewportHeight = 64;
                        viewportSize = { viewportWidth, viewportHeight };
                    }

                    ViewportManager::create(self, 1, origin, viewportSize, view.zoomLevel, view.getPos());

                    self->invalidate();
                }
            }
        }

        // 0x0042A136
        static void sub_42A136(window* self, Gfx::drawpixelinfo_t* dpi, message* news)
        {
            registers regs;
            regs.edi = ToInt(dpi);
            regs.esi = ToInt(self);
            regs.ebp = ToInt(news);
            call(0x0042A136, regs);
        }

        // 0x0042A036
        static void drawViewportString(Gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t y, uint16_t width, uint8_t itemType, uint16_t itemIndex)
        {
            auto args = FormatArguments();

            switch (itemType)
            {
                case newsItemSubTypes::industry:
                {
                    auto industry = IndustryManager::get(itemIndex);
                    args.push(industry->name);
                    args.push(industry->town);
                    break;
                }

                case newsItemSubTypes::station:
                {
                    auto station = StationManager::get(itemIndex);
                    args.push(station->name);
                    args.push(station->town);
                    break;
                }

                case newsItemSubTypes::town:
                {
                    auto town = TownManager::get(itemIndex);
                    args.push(town->name);
                    break;
                }

                case newsItemSubTypes::vehicle:
                {
                    auto vehicle = ThingManager::get<Vehicles::VehicleHead>(itemIndex);
                    auto company = CompanyManager::get(vehicle->owner);
                    if (isPlayerCompany(vehicle->owner))
                    {
                        args.push(StringIds::company_vehicle);
                    }
                    else
                    {
                        args.push(StringIds::competitor_vehicle);
                    }
                    args.push(company->name);
                    args.skip(2);
                    args.push(vehicle->var_22);
                    args.push(vehicle->var_44);
                    break;
                }

                case newsItemSubTypes::company:
                {
                    auto company = CompanyManager::get(itemIndex);
                    args.push(company->name);
                    break;
                }

                case 5:
                case 6:
                    break;

                case newsItemSubTypes::vehicleTab:
                {
                    auto vehicleObj = ObjectManager::get<vehicle_object>(itemIndex);
                    args.push(vehicleObj->name);
                    break;
                }
            }

            switch (itemType)
            {
                case newsItemSubTypes::industry:
                case newsItemSubTypes::station:
                case newsItemSubTypes::town:
                case newsItemSubTypes::vehicle:
                case newsItemSubTypes::company:
                case newsItemSubTypes::vehicleTab:
                {
                    Gfx::drawStringCentredClipped(*dpi, x, y, width, Colour::black, StringIds::black_tiny_font, &args);
                    break;
                }

                case 5:
                case 6:
                    break;
            }
        }

        // 0x00429872
        static void drawLateNews(window* self, Gfx::drawpixelinfo_t* dpi, message* news)
        {
            Gfx::drawImage(dpi, self->x, self->y, ImageIds::news_background_new_left);

            Gfx::drawImage(dpi, self->x + (windowSize.width / 2), self->y, ImageIds::news_background_new_right);

            self->draw(dpi);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            if (!(_word_4F8BE4[news->type] & (1 << 5)))
            {
                *buffer = ControlCodes::font_large;
                buffer++;
            }

            *buffer = ControlCodes::colour_black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 38;
            Gfx::point_t origin = { x, y };

            Gfx::drawStringCentredWrapped(dpi, &origin, 352, Colour::black, StringIds::buffer_2039);

            x = self->x + 1;
            y = self->y + 1;
            origin = { x, y };

            Gfx::drawString_494B3F(*dpi, &origin, Colour::black, StringIds::news_date, &news->date);

            self->drawViewports(dpi);

            sub_42A136(self, dpi, news);
        }

        // 0x00429934
        static void drawMiddleNews(window* self, Gfx::drawpixelinfo_t* dpi, message* news)
        {
            if (_word_4F8BE4[news->type] & (1 << 2))
            {
                if (_word_4F8BE4[news->type] & (1 << 1))
                {
                    if (news->item_id_1 != 0xFFFF)
                    {
                        auto x = self->widgets[Common::widx::viewport1].left + self->x;
                        auto y = self->widgets[Common::widx::viewport1].top + self->y;
                        auto width = self->widgets[Common::widx::viewport1].width() + 1;
                        auto height = self->widgets[Common::widx::viewport1].height() + 1;
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(dpi, x, y, width, height, colour);
                    }
                }
            }

            if (_word_4F8BE4[news->type] & (1 << 3))
            {
                if (_word_4F8BE4[news->type] & (1 << 1))
                {
                    if (news->item_id_2 != 0xFFFF)
                    {
                        auto x = self->widgets[Common::widx::viewport2].left + self->x;
                        auto y = self->widgets[Common::widx::viewport2].top + self->y;
                        auto width = self->widgets[Common::widx::viewport2].width() + 1;
                        auto height = self->widgets[Common::widx::viewport2].height() + 1;
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(dpi, x, y, width, height, colour);
                    }
                }
            }
        }

        // 0x004299E7
        static void drawEarlyNews(window* self, Gfx::drawpixelinfo_t* dpi, message* news)
        {
            auto imageId = Gfx::recolour(ImageIds::news_background_old_left, PaletteIndex::index_68);

            Gfx::drawImage(dpi, self->x, self->y, imageId);

            imageId = Gfx::recolour(ImageIds::news_background_old_right, PaletteIndex::index_68);

            Gfx::drawImage(dpi, self->x + (windowSize.width / 2), self->y, imageId);

            self->draw(dpi);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            if (!(_word_4F8BE4[news->type] & (1 << 5)))
            {
                *buffer = ControlCodes::font_large;
                buffer++;
            }

            *buffer = ControlCodes::colour_black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 38;
            Gfx::point_t origin = { x, y };

            Gfx::drawStringCentredWrapped(dpi, &origin, 352, Colour::black, StringIds::buffer_2039);

            origin.x = self->x + 4;
            origin.y = self->y + 5;

            Gfx::drawString_494B3F(*dpi, &origin, Colour::black, StringIds::news_date, &news->date);

            self->drawViewports(dpi);

            sub_42A136(self, dpi, news);

            x = self->x + 3;
            y = self->y + 5;
            auto width = self->width - 6;
            auto height = self->height;
            auto colour = (1 << 25) | PaletteIndex::index_68;
            Gfx::drawRect(dpi, x, y, width, height, colour);

            x = self->widgets[Common::widx::viewport1].left + self->x;
            y = self->widgets[Common::widx::viewport1].top + self->y;
            width = self->widgets[Common::widx::viewport1].width();
            height = self->widgets[Common::widx::viewport1].height();
            colour = (1 << 25) | PaletteIndex::index_68;
            Gfx::drawRect(dpi, x, y, width, height, colour);
        }

        // 0x00429761
        static void drawStationNews(window* self, Gfx::drawpixelinfo_t* dpi, message* news)
        {
            self->draw(dpi);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer = ControlCodes::colour_black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 17;
            Gfx::point_t origin = { x, y };

            Gfx::drawStringCentredWrapped(dpi, &origin, 338, Colour::black, StringIds::buffer_2039);

            self->drawViewports(dpi);

            if (_word_4F8BE4[news->type] & (1 << 2))
            {
                if (_word_4F8BE4[news->type] & (1 << 1))
                {
                    if (news->item_id_1 != 0xFFFF)
                    {
                        x = self->widgets[Common::widx::viewport1].left + self->x;
                        y = self->widgets[Common::widx::viewport1].top + self->y;
                        auto width = self->widgets[Common::widx::viewport1].width();
                        auto height = self->widgets[Common::widx::viewport1].height();
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(dpi, x, y, width, height, colour);
                    }
                }
            }

            if (_word_4F8BE4[news->type] & (1 << 3))
            {
                if (_word_4F8BE4[news->type] & (1 << 1))
                {
                    if (news->item_id_2 != 0xFFFF)
                    {
                        x = self->widgets[Common::widx::viewport2].left + self->x;
                        y = self->widgets[Common::widx::viewport2].top + self->y;
                        auto width = self->widgets[Common::widx::viewport2].width();
                        auto height = self->widgets[Common::widx::viewport2].height();
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(dpi, x, y, width, height, colour);
                    }
                }
            }
        }

        // 0x00429739
        static void draw(Ui::window* self, Gfx::drawpixelinfo_t* dpi)
        {
            auto news = MessageManager::get(_activeMessageIndex);

            if (_word_4F8BE4[news->type] & (1 << 1))
            {
                if (calcDate(news->date).year >= 1945)
                {
                    drawLateNews(self, dpi, news);

                    if (calcDate(news->date).year < 1985)
                    {
                        drawMiddleNews(self, dpi, news);
                    }
                }
                else
                {
                    drawEarlyNews(self, dpi, news);
                }
            }
            else
            {
                drawStationNews(self, dpi, news);
            }

            if (_word_4F8BE4[news->type] & (1 << 2))
            {
                if (news->item_id_1 != 0xFFFF)
                {
                    auto x = (self->widgets[Common::widx::viewport1Button].left + self->widgets[Common::widx::viewport1Button].right) / 2;
                    x += self->x;
                    auto y = self->widgets[Common::widx::viewport1Button].bottom - 7 + self->y;
                    auto width = self->widgets[Common::widx::viewport1Button].width() - 1;

                    drawViewportString(dpi, x, y, width, _byte_4F8B08[news->type].type, news->item_id_1);
                }
            }
            if (_word_4F8BE4[news->type] & (1 << 3))
            {
                if (news->item_id_2 != 0xFFFF)
                {
                    auto x = (self->widgets[Common::widx::viewport2Button].left + self->widgets[Common::widx::viewport2Button].right) / 2;
                    x += self->x;
                    auto y = self->widgets[Common::widx::viewport2Button].bottom - 7 + self->y;
                    auto width = self->widgets[Common::widx::viewport2Button].width() - 1;

                    drawViewportString(dpi, x, y, width, _byte_4F8B09[news->type].type, news->item_id_2);
                }
            }
        }

        void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = initViewport;
            events.on_update = onUpdate;
            events.viewport_rotate = initViewport;
            events.draw = draw;
        }
    }

    namespace News2
    {
        widget_t widgets[] = {
            commonWidgets(360, 159, widget_type::wt_5),
            widgetEnd(),
        };
    }
}
