#include "News.h"
#include "../../CompanyManager.h"
#include "../../Date.h"
#include "../../Entities/EntityManager.h"
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
#include "../../StationManager.h"
#include "../../TownManager.h"
#include "../../Vehicles/Vehicle.h"
#include "../../ViewportManager.h"
#include "../../Widget.h"
#include "../../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::NewsWindow
{
    namespace News1
    {
        Widget widgets[] = {
            commonWidgets(360, 117, WidgetType::wt_3),
            widgetEnd(),
        };

        WindowEventList events;

        // 0x00429BB7
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                {
                    if (_activeMessageIndex != MessageId::null)
                    {
                        auto news = MessageManager::get(_activeMessageIndex);
                        news->var_C8 = 0xFFFF;
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
                            if (!hasMessageTypeFlag(news->type, MessageTypeFlags::unk2))
                                break;
                        }
                        else
                        {
                            if (!hasMessageTypeFlag(news->type, MessageTypeFlags::unk3))
                                break;
                        }

                        NewsItemSubTypes itemType;
                        uint16_t itemId;
                        if (widgetIndex == Common::widx::viewport1Button)
                        {
                            itemType = getMessageCategories(news->type).type[0];
                            itemId = news->item_id_1;
                        }
                        else
                        {
                            itemType = getMessageCategories(news->type).type[1];
                            itemId = news->item_id_2;
                        }

                        switch (itemType)
                        {
                            case NewsItemSubTypes::industry:
                                Ui::Windows::Industry::open(IndustryId(itemId));
                                break;

                            case NewsItemSubTypes::station:
                                Ui::Windows::Station::open(StationId(itemId));
                                break;

                            case NewsItemSubTypes::town:
                                Ui::Windows::Town::open(itemId);
                                break;

                            case NewsItemSubTypes::vehicle:
                            {
                                auto vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(itemId));

                                Ui::Windows::Vehicle::Main::open(vehicle);
                                break;
                            }

                            case NewsItemSubTypes::company:
                                Ui::Windows::CompanyWindow::open(itemId);
                                break;

                            case NewsItemSubTypes::unk5:
                            case NewsItemSubTypes::unk6:
                                break;

                            case NewsItemSubTypes::vehicleTab:
                                auto vehicleObj = ObjectManager::get<VehicleObject>(itemId);
                                auto window = Ui::Windows::BuildVehicle::open(static_cast<uint32_t>(vehicleObj->type), (1 << 31));
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

                                Ui::Windows::BuildVehicle::sub_4B92A5(window);

                                window->row_hover = rowHover;
                                break;
                        }
                    }
                }
            }
        }

        // 0x00429D2C
        static void onUpdate(Window* self)
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

        static SavedView getView(Window* self, Message* news, uint16_t itemId, NewsItemSubTypes itemType, bool* selectable)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)-1;
            view.thingId = EntityId::null;
            switch (itemType)
            {
                case NewsItemSubTypes::industry:
                {
                    auto industry = IndustryManager::get(IndustryId(itemId));

                    view.mapX = industry->x;
                    view.mapY = industry->y;
                    view.surfaceZ = TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.rotation = gCurrentRotation;
                    view.zoomLevel = ZoomLevel::half;
                    *selectable = true;
                    break;
                }

                case NewsItemSubTypes::station:
                {
                    auto station = StationManager::get(StationId(itemId));

                    view.mapX = station->x;
                    view.mapY = station->y;
                    view.surfaceZ = station->z;
                    view.rotation = gCurrentRotation;
                    view.zoomLevel = ZoomLevel::full;
                    *selectable = true;
                    break;
                }

                case NewsItemSubTypes::town:
                {
                    auto town = TownManager::get(TownId(itemId));

                    view.mapX = town->x;
                    view.mapY = town->y;
                    view.surfaceZ = TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.rotation = gCurrentRotation;
                    view.zoomLevel = ZoomLevel::half;
                    *selectable = true;
                    break;
                }

                case NewsItemSubTypes::vehicle:
                {
                    Vehicles::Vehicle train(EntityId{ itemId });
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

                case NewsItemSubTypes::company:
                    view.zoomLevel = (ZoomLevel)-2;
                    self->invalidate();
                    *selectable = true;
                    break;

                case NewsItemSubTypes::unk5:
                    view.mapX = news->item_id_1; // possible union?
                    view.mapY = news->item_id_2;
                    view.surfaceZ = TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.zoomLevel = ZoomLevel::full;
                    view.rotation = gCurrentRotation;
                    *selectable = true;
                    break;

                case NewsItemSubTypes::unk6:
                    break;

                case NewsItemSubTypes::vehicleTab:
                    view.zoomLevel = (ZoomLevel)-3;
                    self->invalidate();
                    *selectable = true;
                    break;
            }
            return view;
        }

        // 0x00429209
        void initViewport(Window* self)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)-1;
            view.thingId = EntityId::null;
            auto news = MessageManager::get(_activeMessageIndex);

            bool selectable = false;

            if (_activeMessageIndex != MessageId::null)
            {
                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk2))
                {
                    auto itemType = getMessageCategories(news->type).type[0];

                    if (news->item_id_1 != 0xFFFF)
                    {
                        view = getView(self, news, news->item_id_1, itemType, &selectable);
                    }
                }
            }

            self->widgets[Common::widx::viewport1].type = WidgetType::none;
            self->widgets[Common::widx::viewport1Button].type = WidgetType::none;

            if (!view.isEmpty())
            {
                self->widgets[Common::widx::viewport1].type = WidgetType::viewport;
            }

            if (selectable)
            {
                self->widgets[Common::widx::viewport1Button].type = WidgetType::wt_9;
            }

            uint32_t ecx = view.surfaceZ << 16 | view.rotation << 8 | (uint8_t)view.zoomLevel;
            uint32_t edx = view.mapY << 16 | view.mapX | 1 << 30;

            if (!view.isEmpty() && view.isThingView())
            {
                ecx = view.rotation << 8 | (uint8_t)view.zoomLevel;
                edx = enumValue(view.thingId) | view.flags << 16;
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

                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk3))
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
                    Ui::Point origin = { x, y };

                    uint16_t viewportWidth = self->widgets[Common::widx::viewport1].width();
                    uint16_t viewportHeight = 62;
                    Ui::Size viewportSize = { viewportWidth, viewportHeight };

                    if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
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
            view.thingId = EntityId::null;
            selectable = false;

            if (_activeMessageIndex != MessageId::null)
            {
                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk3))
                {
                    auto itemType = getMessageCategories(news->type).type[1];

                    if (news->item_id_2 != 0xFFFF)
                    {
                        view = getView(self, news, news->item_id_2, itemType, &selectable);
                    }
                }
            }

            self->widgets[Common::widx::viewport2].type = WidgetType::none;
            self->widgets[Common::widx::viewport2Button].type = WidgetType::none;

            if (!view.isEmpty())
            {
                self->widgets[Common::widx::viewport2].type = WidgetType::viewport;
            }

            if (selectable)
            {
                self->widgets[Common::widx::viewport2Button].type = WidgetType::wt_9;
            }

            ecx = view.surfaceZ << 16 | view.rotation << 8 | (uint8_t)view.zoomLevel;
            edx = view.mapY << 16 | view.mapX | 1 << 30;

            if (!view.isEmpty() && view.isThingView())
            {
                ecx = view.rotation << 8 | (uint8_t)view.zoomLevel;
                edx = enumValue(view.thingId) | view.flags << 16;
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
                    Ui::Point origin = { x, y };

                    uint16_t viewportWidth = self->widgets[Common::widx::viewport2].width();
                    uint16_t viewportHeight = 62;
                    Ui::Size viewportSize = { viewportWidth, viewportHeight };

                    if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
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
        static void sub_42A136(Window* self, Gfx::Context* context, Message* news)
        {
            registers regs;
            regs.edi = X86Pointer(context);
            regs.esi = X86Pointer(self);
            regs.ebp = X86Pointer(news);
            call(0x0042A136, regs);
        }

        // 0x0042A036
        static void drawViewportString(Gfx::Context* context, uint16_t x, uint16_t y, uint16_t width, NewsItemSubTypes itemType, uint16_t itemIndex)
        {
            auto args = FormatArguments();

            switch (itemType)
            {
                case NewsItemSubTypes::industry:
                {
                    auto industry = IndustryManager::get(IndustryId(itemIndex));
                    args.push(industry->name);
                    args.push(industry->town);
                    break;
                }

                case NewsItemSubTypes::station:
                {
                    auto station = StationManager::get(StationId(itemIndex));
                    args.push(station->name);
                    args.push(station->town);
                    break;
                }

                case NewsItemSubTypes::town:
                {
                    auto town = TownManager::get(TownId(itemIndex));
                    args.push(town->name);
                    break;
                }

                case NewsItemSubTypes::vehicle:
                {
                    auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(itemIndex));
                    auto company = CompanyManager::get(vehicle->owner);
                    if (CompanyManager::isPlayerCompany(vehicle->owner))
                    {
                        args.push(StringIds::company_vehicle);
                    }
                    else
                    {
                        args.push(StringIds::competitor_vehicle);
                    }
                    args.push(company->name);
                    args.skip(2);
                    args.push(vehicle->name);
                    args.push(vehicle->ordinalNumber);
                    break;
                }

                case NewsItemSubTypes::company:
                {
                    auto company = CompanyManager::get(itemIndex);
                    args.push(company->name);
                    break;
                }

                case NewsItemSubTypes::unk5:
                case NewsItemSubTypes::unk6:
                    break;

                case NewsItemSubTypes::vehicleTab:
                {
                    auto vehicleObj = ObjectManager::get<VehicleObject>(itemIndex);
                    args.push(vehicleObj->name);
                    break;
                }
            }

            switch (itemType)
            {
                case NewsItemSubTypes::industry:
                case NewsItemSubTypes::station:
                case NewsItemSubTypes::town:
                case NewsItemSubTypes::vehicle:
                case NewsItemSubTypes::company:
                case NewsItemSubTypes::vehicleTab:
                {
                    Gfx::drawStringCentredClipped(*context, x, y, width, Colour::black, StringIds::black_tiny_font, &args);
                    break;
                }

                case NewsItemSubTypes::unk5:
                case NewsItemSubTypes::unk6:
                    break;
            }
        }

        // 0x00429872
        static void drawLateNews(Window* self, Gfx::Context* context, Message* news)
        {
            Gfx::drawImage(context, self->x, self->y, ImageIds::news_background_new_left);

            Gfx::drawImage(context, self->x + (windowSize.width / 2), self->y, ImageIds::news_background_new_right);

            self->draw(context);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            if (!hasMessageTypeFlag(news->type, MessageTypeFlags::unk5))
            {
                *buffer = ControlCodes::font_large;
                buffer++;
            }

            *buffer = ControlCodes::colour_black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 38;
            Ui::Point origin = { x, y };

            Gfx::drawStringCentredWrapped(*context, origin, 352, Colour::black, StringIds::buffer_2039);

            x = self->x + 1;
            y = self->y + 1;
            origin = { x, y };

            Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::news_date, &news->date);

            self->drawViewports(context);

            sub_42A136(self, context, news);
        }

        // 0x00429934
        static void drawMiddleNews(Window* self, Gfx::Context* context, Message* news)
        {
            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk2))
            {
                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
                {
                    if (news->item_id_1 != 0xFFFF)
                    {
                        auto x = self->widgets[Common::widx::viewport1].left + self->x;
                        auto y = self->widgets[Common::widx::viewport1].top + self->y;
                        auto width = self->widgets[Common::widx::viewport1].width() + 1;
                        auto height = self->widgets[Common::widx::viewport1].height() + 1;
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(*context, x, y, width, height, colour);
                    }
                }
            }

            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk3))
            {
                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
                {
                    if (news->item_id_2 != 0xFFFF)
                    {
                        auto x = self->widgets[Common::widx::viewport2].left + self->x;
                        auto y = self->widgets[Common::widx::viewport2].top + self->y;
                        auto width = self->widgets[Common::widx::viewport2].width() + 1;
                        auto height = self->widgets[Common::widx::viewport2].height() + 1;
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(*context, x, y, width, height, colour);
                    }
                }
            }
        }

        // 0x004299E7
        static void drawEarlyNews(Window* self, Gfx::Context* context, Message* news)
        {
            auto imageId = Gfx::recolour(ImageIds::news_background_old_left, PaletteIndex::index_68);

            Gfx::drawImage(context, self->x, self->y, imageId);

            imageId = Gfx::recolour(ImageIds::news_background_old_right, PaletteIndex::index_68);

            Gfx::drawImage(context, self->x + (windowSize.width / 2), self->y, imageId);

            self->draw(context);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            if (!hasMessageTypeFlag(news->type, MessageTypeFlags::unk5))
            {
                *buffer = ControlCodes::font_large;
                buffer++;
            }

            *buffer = ControlCodes::colour_black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 38;
            Ui::Point origin = { x, y };

            Gfx::drawStringCentredWrapped(*context, origin, 352, Colour::black, StringIds::buffer_2039);

            origin.x = self->x + 4;
            origin.y = self->y + 5;

            Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::news_date, &news->date);

            self->drawViewports(context);

            sub_42A136(self, context, news);

            x = self->x + 3;
            y = self->y + 5;
            auto width = self->width - 6;
            auto height = self->height;
            auto colour = (1 << 25) | PaletteIndex::index_68;
            Gfx::drawRect(*context, x, y, width, height, colour);

            x = self->widgets[Common::widx::viewport1].left + self->x;
            y = self->widgets[Common::widx::viewport1].top + self->y;
            width = self->widgets[Common::widx::viewport1].width();
            height = self->widgets[Common::widx::viewport1].height();
            colour = (1 << 25) | PaletteIndex::index_68;
            Gfx::drawRect(*context, x, y, width, height, colour);
        }

        // 0x00429761
        static void drawStationNews(Window* self, Gfx::Context* context, Message* news)
        {
            self->draw(context);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer = ControlCodes::colour_black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 17;
            Ui::Point origin = { x, y };

            Gfx::drawStringCentredWrapped(*context, origin, 338, Colour::black, StringIds::buffer_2039);

            self->drawViewports(context);

            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk2))
            {
                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
                {
                    if (news->item_id_1 != 0xFFFF)
                    {
                        x = self->widgets[Common::widx::viewport1].left + self->x;
                        y = self->widgets[Common::widx::viewport1].top + self->y;
                        auto width = self->widgets[Common::widx::viewport1].width();
                        auto height = self->widgets[Common::widx::viewport1].height();
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(*context, x, y, width, height, colour);
                    }
                }
            }

            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk3))
            {
                if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
                {
                    if (news->item_id_2 != 0xFFFF)
                    {
                        x = self->widgets[Common::widx::viewport2].left + self->x;
                        y = self->widgets[Common::widx::viewport2].top + self->y;
                        auto width = self->widgets[Common::widx::viewport2].width();
                        auto height = self->widgets[Common::widx::viewport2].height();
                        auto colour = (1 << 25) | PaletteIndex::index_35;
                        Gfx::drawRect(*context, x, y, width, height, colour);
                    }
                }
            }
        }

        // 0x00429739
        static void draw(Ui::Window* self, Gfx::Context* context)
        {
            auto news = MessageManager::get(_activeMessageIndex);

            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk1))
            {
                if (calcDate(news->date).year >= 1945)
                {
                    drawLateNews(self, context, news);

                    if (calcDate(news->date).year < 1985)
                    {
                        drawMiddleNews(self, context, news);
                    }
                }
                else
                {
                    drawEarlyNews(self, context, news);
                }
            }
            else
            {
                drawStationNews(self, context, news);
            }

            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk2))
            {
                if (news->item_id_1 != 0xFFFF)
                {
                    auto x = (self->widgets[Common::widx::viewport1Button].left + self->widgets[Common::widx::viewport1Button].right) / 2;
                    x += self->x;
                    auto y = self->widgets[Common::widx::viewport1Button].bottom - 7 + self->y;
                    auto width = self->widgets[Common::widx::viewport1Button].width() - 1;

                    drawViewportString(context, x, y, width, getMessageCategories(news->type).type[0], news->item_id_1);
                }
            }
            if (hasMessageTypeFlag(news->type, MessageTypeFlags::unk3))
            {
                if (news->item_id_2 != 0xFFFF)
                {
                    auto x = (self->widgets[Common::widx::viewport2Button].left + self->widgets[Common::widx::viewport2Button].right) / 2;
                    x += self->x;
                    auto y = self->widgets[Common::widx::viewport2Button].bottom - 7 + self->y;
                    auto width = self->widgets[Common::widx::viewport2Button].width() - 1;

                    drawViewportString(context, x, y, width, getMessageCategories(news->type).type[1], news->item_id_2);
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
        Widget widgets[] = {
            commonWidgets(360, 159, WidgetType::wt_5),
            widgetEnd(),
        };
    }
}
