#include "News.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Message.h"
#include "MessageManager.h"
#include "Objects/CompetitorObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/Window.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleDraw.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::NewsWindow
{
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    static loco_global<int8_t[8], 0x011364F0> _trackTypesForTab;

    namespace News1
    {
        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets<Widgets::Wt3Widget>(360, 117)

        );

        std::span<const Widget> getWidgets()
        {
            return widgets;
        }

        // 0x00429BB7
        static void onMouseUp([[maybe_unused]] Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                {
                    MessageManager::clearActiveMessage();
                    break;
                }

                case Common::widx::viewport1Button:
                case Common::widx::viewport2Button:
                {
                    if (MessageManager::getActiveIndex() != MessageId::null)
                    {
                        auto news = MessageManager::get(MessageManager::getActiveIndex());
                        const auto& mtd = getMessageTypeDescriptor(news->type);
                        if (widgetIndex == Common::widx::viewport1Button)
                        {
                            if (!mtd.hasFlag(MessageTypeFlags::hasFirstItem))
                            {
                                break;
                            }
                        }
                        else
                        {
                            if (!mtd.hasFlag(MessageTypeFlags::hasSecondItem))
                            {
                                break;
                            }
                        }

                        MessageItemArgumentType itemType;
                        uint16_t itemId;
                        if (widgetIndex == Common::widx::viewport1Button)
                        {
                            itemType = mtd.argumentTypes[0];
                            itemId = news->itemSubjects[0];
                        }
                        else
                        {
                            itemType = mtd.argumentTypes[1];
                            itemId = news->itemSubjects[1];
                        }

                        switch (itemType)
                        {
                            case MessageItemArgumentType::industry:
                                Ui::Windows::Industry::open(IndustryId(itemId));
                                break;

                            case MessageItemArgumentType::station:
                                Ui::Windows::Station::open(StationId(itemId));
                                break;

                            case MessageItemArgumentType::town:
                                Ui::Windows::Town::open(itemId);
                                break;

                            case MessageItemArgumentType::vehicle:
                            {
                                auto vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(itemId));

                                Ui::Windows::Vehicle::Main::open(vehicle);
                                break;
                            }

                            case MessageItemArgumentType::company:
                                Ui::Windows::CompanyWindow::open(CompanyId(itemId));
                                break;

                            case MessageItemArgumentType::location:
                            case MessageItemArgumentType::unk6:
                            case MessageItemArgumentType::null:
                                break;

                            case MessageItemArgumentType::vehicleTab:
                                auto vehicleObj = ObjectManager::get<VehicleObject>(itemId);
                                auto window = Ui::Windows::BuildVehicle::open(static_cast<uint32_t>(vehicleObj->type), (1U << 31));
                                window->rowHover = itemId;
                                if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
                                {
                                    if (vehicleObj->trackType != 0xFF)
                                    {
                                        for (uint8_t i = 0; i < _numTrackTypeTabs && i < std::size(_trackTypesForTab); ++i)
                                        {
                                            if (vehicleObj->trackType == _trackTypesForTab[i])
                                            {
                                                window->currentSecondaryTab = i;
                                                break;
                                            }
                                        }
                                    }
                                }

                                auto rowHover = window->rowHover;

                                Ui::Windows::BuildVehicle::sub_4B92A5(window);

                                window->rowHover = rowHover;
                                break;
                        }
                    }
                }
            }
        }

        // 0x00429D2C
        static void onUpdate(Window& self)
        {
            uint16_t height = _nState.slideInHeight + 4;

            _nState.slideInHeight = std::min(height, self.height);

            height = Ui::height() - _nState.slideInHeight - self.y;
            auto width = (Ui::width() / 2) - (kWindowSize.width / 2) - self.x;

            if (width != 0 || height != 0)
            {
                self.invalidate();
                self.y += height;
                self.x += width;

                if (self.viewports[0] != nullptr)
                {
                    self.viewports[0]->x += width;
                    self.viewports[0]->y += height;
                }

                if (self.viewports[1] != nullptr)
                {
                    self.viewports[1]->x += width;
                    self.viewports[1]->y += height;
                }

                self.invalidate();
            }
        }

        static SavedView getView(Window* self, Message* news, uint16_t itemId, MessageItemArgumentType itemType, bool* selectable)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)0xFFU;
            view.entityId = EntityId::null;
            switch (itemType)
            {
                case MessageItemArgumentType::industry:
                {
                    auto industry = IndustryManager::get(IndustryId(itemId));

                    view.mapX = industry->x;
                    view.mapY = industry->y;
                    view.surfaceZ = World::TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.rotation = WindowManager::getCurrentRotation();
                    view.zoomLevel = ZoomLevel::half;
                    *selectable = true;
                    break;
                }

                case MessageItemArgumentType::station:
                {
                    auto station = StationManager::get(StationId(itemId));

                    view.mapX = station->x;
                    view.mapY = station->y;
                    view.surfaceZ = station->z;
                    view.rotation = WindowManager::getCurrentRotation();
                    view.zoomLevel = ZoomLevel::full;
                    *selectable = true;
                    break;
                }

                case MessageItemArgumentType::town:
                {
                    auto town = TownManager::get(TownId(itemId));

                    view.mapX = town->x;
                    view.mapY = town->y;
                    view.surfaceZ = World::TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.rotation = WindowManager::getCurrentRotation();
                    view.zoomLevel = ZoomLevel::half;
                    *selectable = true;
                    break;
                }

                case MessageItemArgumentType::vehicle:
                {
                    auto* head = EntityManager::get<Vehicles::VehicleHead>(EntityId(itemId));
                    if (head == nullptr)
                    {
                        break;
                    }
                    Vehicles::Vehicle train(*head);
                    if (train.head->tileX == -1)
                    {
                        break;
                    }

                    view.entityId = train.veh2->id;

                    if (!train.cars.empty())
                    {
                        view.entityId = train.cars.firstCar.body->id;
                    }

                    view.flags = (1 << 15);
                    view.zoomLevel = ZoomLevel::full;
                    view.rotation = WindowManager::getCurrentRotation();
                    *selectable = true;
                    break;
                }

                case MessageItemArgumentType::company:
                    // Used to indicate to drawNewsSubjectImages to draw a company image
                    // TODO: Do this better
                    view.zoomLevel = enumValue(SubjectType::companyFace);
                    self->invalidate();
                    *selectable = true;
                    break;

                case MessageItemArgumentType::location:
                    view.mapX = news->itemSubjects[0]; // possible union?
                    view.mapY = news->itemSubjects[1];
                    view.surfaceZ = World::TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.zoomLevel = ZoomLevel::full;
                    view.rotation = WindowManager::getCurrentRotation();
                    *selectable = true;
                    break;

                case MessageItemArgumentType::unk6:
                case MessageItemArgumentType::null:
                    break;

                case MessageItemArgumentType::vehicleTab:
                    // Used to indicate to drawNewsSubjectImages to draw a vehicle image
                    // TODO: Do this better
                    view.zoomLevel = enumValue(SubjectType::vehicleImage);
                    self->invalidate();
                    *selectable = true;
                    break;
            }
            return view;
        }

        // TODO: deduplicate with initViewport1
        static void initViewport0(Window& self)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)0xFFU;
            view.entityId = EntityId::null;

            auto news = MessageManager::get(MessageManager::getActiveIndex());
            const auto& mtd = getMessageTypeDescriptor(news->type);

            bool selectable = false;

            if (MessageManager::getActiveIndex() != MessageId::null)
            {
                if (mtd.hasFlag(MessageTypeFlags::hasFirstItem))
                {
                    auto itemType = mtd.argumentTypes[0];

                    if (news->itemSubjects[0] != 0xFFFF)
                    {
                        view = getView(&self, news, news->itemSubjects[0], itemType, &selectable);
                    }
                }
            }

            self.widgets[Common::widx::viewport1].hidden = true;
            self.widgets[Common::widx::viewport1Button].hidden = true;

            if (!view.isEmpty())
            {
                self.widgets[Common::widx::viewport1].hidden = false;
            }

            if (selectable)
            {
                self.widgets[Common::widx::viewport1Button].hidden = false;
            }

            if (_nState.savedView[0] != view)
            {
                _nState.savedView[0] = view;
                self.viewportRemove(0);
                self.invalidate();

                self.widgets[Common::widx::viewport1].left = 6;
                self.widgets[Common::widx::viewport1].right = 353;
                self.widgets[Common::widx::viewport1Button].left = 4;
                self.widgets[Common::widx::viewport1Button].right = 355;

                if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
                {
                    self.widgets[Common::widx::viewport1].left = 6;
                    self.widgets[Common::widx::viewport1].right = 173;
                    self.widgets[Common::widx::viewport1Button].left = 4;
                    self.widgets[Common::widx::viewport1Button].right = 175;
                }

                if (!view.isEmpty())
                {
                    int16_t x = self.widgets[Common::widx::viewport1].left + 1 + self.x;
                    int16_t y = self.widgets[Common::widx::viewport1].top + 1 + self.y;
                    Ui::Point origin = { x, y };

                    uint16_t viewportWidth = self.widgets[Common::widx::viewport1].width();
                    uint16_t viewportHeight = 62;
                    Ui::Size viewportSize = { viewportWidth, viewportHeight };

                    if (mtd.hasFlag(MessageTypeFlags::unk1))
                    {
                        x = self.widgets[Common::widx::viewport1].left + self.x;
                        y = self.widgets[Common::widx::viewport1].top + self.y;
                        origin = { x, y };

                        viewportWidth = self.widgets[Common::widx::viewport1].width() + 2;
                        viewportHeight = 64;
                        viewportSize = { viewportWidth, viewportHeight };
                    }

                    if (view.isEntityView())
                    {
                        ViewportManager::create(&self, 0, origin, viewportSize, view.zoomLevel, view.entityId);
                    }
                    else
                    {
                        ViewportManager::create(&self, 0, origin, viewportSize, view.zoomLevel, view.getPos());
                    }
                    self.invalidate();
                }
            }
        }

        // TODO: deduplicate with initViewport0
        static void initViewport1(Window& self)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)0xFFU;
            view.entityId = EntityId::null;

            auto news = MessageManager::get(MessageManager::getActiveIndex());
            const auto& mtd = getMessageTypeDescriptor(news->type);

            bool selectable = false;

            if (MessageManager::getActiveIndex() != MessageId::null)
            {
                if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
                {
                    auto itemType = mtd.argumentTypes[1];

                    if (news->itemSubjects[1] != 0xFFFF)
                    {
                        view = getView(&self, news, news->itemSubjects[1], itemType, &selectable);
                    }
                }
            }

            self.widgets[Common::widx::viewport2].hidden = true;
            self.widgets[Common::widx::viewport2Button].hidden = true;

            if (!view.isEmpty())
            {
                self.widgets[Common::widx::viewport2].hidden = false;
            }

            if (selectable)
            {
                self.widgets[Common::widx::viewport2Button].hidden = false;
            }

            if (_nState.savedView[1] != view)
            {
                _nState.savedView[1] = view;
                self.viewportRemove(1);
                self.invalidate();

                self.widgets[Common::widx::viewport2].left = 186;
                self.widgets[Common::widx::viewport2].right = 353;
                self.widgets[Common::widx::viewport2Button].left = 184;
                self.widgets[Common::widx::viewport2Button].right = 355;

                if (!view.isEmpty())
                {
                    int16_t x = self.widgets[Common::widx::viewport2].left + 1 + self.x;
                    int16_t y = self.widgets[Common::widx::viewport2].top + 1 + self.y;
                    Ui::Point origin = { x, y };

                    uint16_t viewportWidth = self.widgets[Common::widx::viewport2].width();
                    uint16_t viewportHeight = 62;
                    Ui::Size viewportSize = { viewportWidth, viewportHeight };

                    if (mtd.hasFlag(MessageTypeFlags::unk1))
                    {
                        x = self.widgets[Common::widx::viewport2].left + self.x;
                        y = self.widgets[Common::widx::viewport2].top + self.y;
                        origin = { x, y };

                        viewportWidth = self.widgets[Common::widx::viewport2].width() + 2;
                        viewportHeight = 64;
                        viewportSize = { viewportWidth, viewportHeight };
                    }

                    ViewportManager::create(&self, 1, origin, viewportSize, view.zoomLevel, view.getPos());

                    self.invalidate();
                }
            }
        }

        // 0x00429209
        void initViewports(Window& self)
        {
            initViewport0(self);
            initViewport1(self);
        }

        // 0x0042A136
        static void drawNewsSubjectImages(Window* self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            for (auto i = 0; i < 2; ++i)
            {
                const auto itemSubject = news->itemSubjects[i];
                const auto& viewWidget = self->widgets[Common::widx::viewport1 + i];
                const SubjectType subjectType = SubjectType((uint8_t)_nState.savedView[i].zoomLevel);

                if (subjectType == SubjectType::companyFace && itemSubject != 0xFFFFU)
                {
                    const auto* company = CompanyManager::get(CompanyId(itemSubject));
                    const auto* competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);
                    const auto imageIndexBase = competitorObj->images[enumValue(company->ownerEmotion)];

                    const ImageId imageId(imageIndexBase + 1, company->mainColours.primary);
                    const auto x = self->x + viewWidget.midX() - 31;
                    const auto y = self->y + viewWidget.midY() - 31;
                    drawingCtx.drawImage(Ui::Point(x, y), imageId);

                    if (company->jailStatus != 0)
                    {
                        drawingCtx.drawImage(Ui::Point(x, y), ImageId(ImageIds::owner_jailed));
                    }
                }

                if (subjectType == SubjectType::vehicleImage && itemSubject != 0xFFFFU)
                {
                    const auto x = self->x + viewWidget.left;
                    const auto y = self->y + viewWidget.top;

                    const auto& rt = drawingCtx.currentRenderTarget();
                    auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(x + 1, y + 1, viewWidget.width() - 2, viewWidget.height() - 2));
                    if (clipped)
                    {
                        drawingCtx.pushRenderTarget(*clipped);

                        drawVehicleOverview(
                            drawingCtx,
                            { viewWidget.midX(), 35 },
                            itemSubject,
                            Ui::WindowManager::getVehiclePreviewRotationFrameYaw(),
                            Ui::WindowManager::getVehiclePreviewRotationFrameRoll(),
                            CompanyManager::getControllingId());

                        drawingCtx.popRenderTarget();
                    }
                }
            }
        }

        // 0x0042A036
        static void drawViewportString(Gfx::DrawingContext& drawingCtx, Point origin, uint16_t width, MessageItemArgumentType itemType, uint16_t itemIndex)
        {
            FormatArguments args{};

            switch (itemType)
            {
                case MessageItemArgumentType::industry:
                {
                    auto industry = IndustryManager::get(IndustryId(itemIndex));
                    args.push(industry->name);
                    args.push(industry->town);
                    break;
                }

                case MessageItemArgumentType::station:
                {
                    auto station = StationManager::get(StationId(itemIndex));
                    args.push(station->name);
                    args.push(station->town);
                    break;
                }

                case MessageItemArgumentType::town:
                {
                    auto town = TownManager::get(TownId(itemIndex));
                    args.push(town->name);
                    break;
                }

                case MessageItemArgumentType::vehicle:
                {
                    auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(itemIndex));
                    if (vehicle == nullptr)
                    {
                        // We could close the window now but then we can't view old news about removed vehicles.
                        return;
                    }
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

                case MessageItemArgumentType::company:
                {
                    auto company = CompanyManager::get(CompanyId(itemIndex));
                    args.push(company->name);
                    break;
                }

                case MessageItemArgumentType::location:
                case MessageItemArgumentType::unk6:
                case MessageItemArgumentType::null:
                    break;

                case MessageItemArgumentType::vehicleTab:
                {
                    auto vehicleObj = ObjectManager::get<VehicleObject>(itemIndex);
                    args.push(vehicleObj->name);
                    break;
                }
            }

            switch (itemType)
            {
                case MessageItemArgumentType::industry:
                case MessageItemArgumentType::station:
                case MessageItemArgumentType::town:
                case MessageItemArgumentType::vehicle:
                case MessageItemArgumentType::company:
                case MessageItemArgumentType::vehicleTab:
                {
                    auto tr = Gfx::TextRenderer(drawingCtx);
                    tr.drawStringCentredClipped(origin, width, Colour::black, StringIds::black_tiny_font, args);
                    break;
                }

                case MessageItemArgumentType::location:
                case MessageItemArgumentType::unk6:
                case MessageItemArgumentType::null:
                    break;
            }
        }

        // 0x00429872
        static void drawLateNews(Window* self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            drawingCtx.drawImage(self->x, self->y, ImageIds::news_background_new_left);

            drawingCtx.drawImage(self->x + (kWindowSize.width / 2), self->y, ImageIds::news_background_new_right);

            self->draw(drawingCtx);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            const auto& mtd = getMessageTypeDescriptor(news->type);

            if (!mtd.hasFlag(MessageTypeFlags::unk5))
            {
                tr.setCurrentFont(Gfx::Font::large);
            }

            *buffer = ControlCodes::Colour::black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 38;
            Ui::Point origin = { x, y };

            tr.drawStringCentredWrapped(origin, 352, Colour::black, StringIds::buffer_2039);

            x = self->x + 1;
            y = self->y + 1;
            origin = { x, y };

            auto argsBuf = FormatArgumentsBuffer{};
            auto args = FormatArguments{ argsBuf };
            args.push(news->date);
            tr.drawStringLeft(origin, Colour::black, StringIds::news_date, args);

            drawNewsSubjectImages(self, drawingCtx, news);
        }

        // 0x00429934
        static void drawMiddleNews(Window* self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            const auto& mtd = getMessageTypeDescriptor(news->type);
            if (mtd.hasFlag(MessageTypeFlags::hasFirstItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::unk1))
                {
                    if (news->itemSubjects[0] != 0xFFFF)
                    {
                        auto x = self->widgets[Common::widx::viewport1].left + self->x;
                        auto y = self->widgets[Common::widx::viewport1].top + self->y;
                        auto width = self->widgets[Common::widx::viewport1].width() + 1;
                        auto height = self->widgets[Common::widx::viewport1].height() + 1;
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }

            if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::unk1))
                {
                    if (news->itemSubjects[1] != 0xFFFF)
                    {
                        auto x = self->widgets[Common::widx::viewport2].left + self->x;
                        auto y = self->widgets[Common::widx::viewport2].top + self->y;
                        auto width = self->widgets[Common::widx::viewport2].width() + 1;
                        auto height = self->widgets[Common::widx::viewport2].height() + 1;
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }
        }

        // 0x004299E7
        static void drawEarlyNews(Window* self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            auto imageId = Gfx::recolour(ImageIds::news_background_old_left, ExtColour::translucentBrown1);
            drawingCtx.drawImage(self->x, self->y, imageId);

            imageId = Gfx::recolour(ImageIds::news_background_old_right, ExtColour::translucentBrown1);

            drawingCtx.drawImage(self->x + (kWindowSize.width / 2), self->y, imageId);

            self->draw(drawingCtx);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            const auto& mtd = getMessageTypeDescriptor(news->type);

            if (!mtd.hasFlag(MessageTypeFlags::unk5))
            {
                tr.setCurrentFont(Gfx::Font::large);
            }

            *buffer = ControlCodes::Colour::black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 38;
            Ui::Point origin = { x, y };

            tr.drawStringCentredWrapped(origin, 352, Colour::black, StringIds::buffer_2039);

            origin.x = self->x + 4;
            origin.y = self->y + 5;

            auto argsBuf = FormatArgumentsBuffer{};
            auto args = FormatArguments{ argsBuf };
            args.push(news->date);
            tr.drawStringLeft(origin, Colour::black, StringIds::news_date, args);

            drawNewsSubjectImages(self, drawingCtx, news);

            x = self->x + 3;
            y = self->y + 5;
            auto width = self->width - 6;
            auto height = self->height;
            auto colour = enumValue(ExtColour::translucentBrown1);
            drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);

            x = self->widgets[Common::widx::viewport1].left + self->x;
            y = self->widgets[Common::widx::viewport1].top + self->y;
            width = self->widgets[Common::widx::viewport1].width();
            height = self->widgets[Common::widx::viewport1].height();
            colour = enumValue(ExtColour::translucentBrown1);
            drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
        }

        // 0x00429761
        static void drawStationNews(Window* self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self->draw(drawingCtx);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer = ControlCodes::Colour::black;
            buffer++;

            strncpy(buffer, newsString, 512);

            int16_t x = (self->width / 2) + self->x;
            int16_t y = self->y + 17;
            Ui::Point origin = { x, y };

            tr.drawStringCentredWrapped(origin, 338, Colour::black, StringIds::buffer_2039);

            const auto& mtd = getMessageTypeDescriptor(news->type);
            if (mtd.hasFlag(MessageTypeFlags::hasFirstItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::unk1))
                {
                    if (news->itemSubjects[0] != 0xFFFF)
                    {
                        x = self->widgets[Common::widx::viewport1].left + self->x;
                        y = self->widgets[Common::widx::viewport1].top + self->y;
                        auto width = self->widgets[Common::widx::viewport1].width();
                        auto height = self->widgets[Common::widx::viewport1].height();
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }

            if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::unk1))
                {
                    if (news->itemSubjects[1] != 0xFFFF)
                    {
                        x = self->widgets[Common::widx::viewport2].left + self->x;
                        y = self->widgets[Common::widx::viewport2].top + self->y;
                        auto width = self->widgets[Common::widx::viewport2].width();
                        auto height = self->widgets[Common::widx::viewport2].height();
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }
        }

        // 0x00429739
        static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto news = MessageManager::get(MessageManager::getActiveIndex());
            const auto& mtd = getMessageTypeDescriptor(news->type);

            if (mtd.hasFlag(MessageTypeFlags::unk1))
            {
                if (calcDate(news->date).year >= 1945)
                {
                    drawLateNews(&self, drawingCtx, news);

                    if (calcDate(news->date).year < 1985)
                    {
                        drawMiddleNews(&self, drawingCtx, news);
                    }
                }
                else
                {
                    drawEarlyNews(&self, drawingCtx, news);
                }
            }
            else
            {
                drawStationNews(&self, drawingCtx, news);
            }

            if (mtd.hasFlag(MessageTypeFlags::hasFirstItem))
            {
                if (news->itemSubjects[0] != 0xFFFF)
                {
                    auto x = (self.widgets[Common::widx::viewport1Button].left + self.widgets[Common::widx::viewport1Button].right) / 2;
                    x += self.x;
                    auto y = self.widgets[Common::widx::viewport1Button].bottom - 7 + self.y;
                    auto width = self.widgets[Common::widx::viewport1Button].width() - 1;
                    auto point = Point(x, y);

                    drawViewportString(drawingCtx, point, width, mtd.argumentTypes[0], news->itemSubjects[0]);
                }
            }
            if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
            {
                if (news->itemSubjects[1] != 0xFFFF)
                {
                    auto x = (self.widgets[Common::widx::viewport2Button].left + self.widgets[Common::widx::viewport2Button].right) / 2;
                    x += self.x;
                    auto y = self.widgets[Common::widx::viewport2Button].bottom - 7 + self.y;
                    auto width = self.widgets[Common::widx::viewport2Button].width() - 1;
                    auto point = Point(x, y);

                    drawViewportString(drawingCtx, point, width, mtd.argumentTypes[1], news->itemSubjects[1]);
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = initViewports,
            .onUpdate = onUpdate,
            .viewportRotate = initViewports,
            .draw = draw,
        };

        const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace News2
    {
        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets<Widgets::Panel>(360, 159)

        );

        std::span<const Widget> getWidgets()
        {
            return widgets;
        }
    }
}
