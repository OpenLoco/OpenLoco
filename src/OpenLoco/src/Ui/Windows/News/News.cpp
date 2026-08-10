#include "Ui/Windows/News/News.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
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
#include "Ui/Widgets/NewsPanelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/Window.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleBody.h"
#include "Vehicles/VehicleDraw.h"
#include "Vehicles/VehicleHead.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

namespace OpenLoco::Ui::Windows::NewsWindow
{
    namespace News1
    {
        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets<Widgets::Panel>(360, 117)

        );

        std::span<const Widget> getWidgets()
        {
            return widgets;
        }
    }

    namespace News2
    {
        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets<Widgets::NewsPanel>(360, 159)

        );

        std::span<const Widget> getWidgets()
        {
            return widgets;
        }
    }

    namespace Common
    {
        // 0x00429BB7
        static void onMouseUp([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, const WidgetId id)
        {
            switch (id)
            {
                case Common::Widx::kCloseButton:
                {
                    MessageManager::clearActiveMessage();
                    break;
                }

                case Common::Widx::kViewport1Button:
                case Common::Widx::kViewport2Button:
                {
                    if (MessageManager::getActiveIndex() != MessageId::null)
                    {
                        auto news = MessageManager::get(MessageManager::getActiveIndex());
                        const auto& mtd = getMessageTypeDescriptor(news->type);
                        if (id == Common::Widx::kViewport1Button)
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
                        if (id == Common::Widx::kViewport1Button)
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
                                Ui::Windows::BuildVehicle::openByVehicleObjectId(itemId);
                                break;
                        }
                    }
                }
            }
        }

        // 0x00429D2C
        static void onUpdate(Window& self)
        {
            auto height = _nState.slideInHeight + 4;

            _nState.slideInHeight = std::min(height, self.height);

            height = Ui::height() - _nState.slideInHeight - self.y;
            auto width = (Ui::width() / 2) - (self.width / 2) - self.x;

            if (width != 0 || height != 0)
            {
                self.invalidate();
                self.y += height;
                self.x += width;
                self.invalidate();
            }

            for (auto i = 0; i < 2; ++i)
            {
                const auto subjectType = SubjectType(static_cast<uint8_t>(static_cast<int8_t>(_nState.savedView[i].zoomLevel)));
                if (subjectType == SubjectType::vehicleImage)
                {
                    WindowManager::invalidateWidget(WindowType::news, self.number, Common::widx::viewport1 + i);
                }
            }
        }

        static SavedView getView(Window* self, const Message* news, uint16_t itemId, MessageItemArgumentType itemType, bool& selectable)
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
                    selectable = true;
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
                    selectable = true;
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
                    selectable = true;
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
                    selectable = true;
                    break;
                }

                case MessageItemArgumentType::company:
                    // Used to indicate to drawNewsSubjectImages to draw a company image
                    // TODO: Do this better
                    view.zoomLevel = enumValue(SubjectType::companyFace);
                    self->invalidate();
                    selectable = true;
                    break;

                case MessageItemArgumentType::location:
                    view.mapX = news->itemSubjects[0]; // possible union?
                    view.mapY = news->itemSubjects[1];
                    view.surfaceZ = World::TileManager::getHeight({ view.mapX, view.mapY }).landHeight;
                    view.zoomLevel = ZoomLevel::full;
                    view.rotation = WindowManager::getCurrentRotation();
                    selectable = true;
                    break;

                case MessageItemArgumentType::unk6:
                case MessageItemArgumentType::null:
                    break;

                case MessageItemArgumentType::vehicleTab:
                    // Used to indicate to drawNewsSubjectImages to draw a vehicle image
                    // TODO: Do this better
                    view.zoomLevel = enumValue(SubjectType::vehicleImage);
                    self->invalidate();
                    selectable = true;
                    break;
            }
            return view;
        }

        struct ViewportLayout
        {
            MessageTypeFlags subjectFlag;
            WidgetIndex_t viewportWidgetId;
            WidgetIndex_t buttonWidgetId;
            Point position;
            Size fullSize;
            Size halfSize;
        };

        static constexpr Size kFullSizeNews = { 351, 42 };
        static constexpr Size kHalfSizeNews = { 174, 42 };

        static constexpr std::array kViewportLayouts = std::to_array<ViewportLayout>({
            { MessageTypeFlags::hasFirstItem, Common::widx::viewport1, Common::widx::viewport1Button, Point{ 4, 42 }, kFullSizeNews, kHalfSizeNews },
            { MessageTypeFlags::hasSecondItem, Common::widx::viewport2, Common::widx::viewport2Button, Point{ 186, 42 }, kHalfSizeNews, kHalfSizeNews },
        });

        static void initViewport(Window& self, const uint8_t subjectIndex)
        {
            SavedView view;
            view.mapX = -1;
            view.mapY = -1;
            view.surfaceZ = -1;
            view.rotation = -1;
            view.zoomLevel = (ZoomLevel)0xFFU;
            view.entityId = EntityId::null;

            const auto* news = MessageManager::get(MessageManager::getActiveIndex());
            const auto& mtd = getMessageTypeDescriptor(news->type);
            const auto& layout = kViewportLayouts[subjectIndex];

            bool selectable = false;
            if (MessageManager::getActiveIndex() != MessageId::null)
            {
                if (mtd.hasFlag(layout.subjectFlag))
                {
                    auto itemType = mtd.argumentTypes[subjectIndex];

                    if (news->itemSubjects[subjectIndex] != 0xFFFF)
                    {
                        view = getView(&self, news, news->itemSubjects[subjectIndex], itemType, selectable);
                    }
                }
            }

            auto& viewportWidget = self.widgets[layout.viewportWidgetId];
            auto& buttonWidget = self.widgets[layout.buttonWidgetId];

            viewportWidget.hidden = view.isEmpty();
            buttonWidget.hidden = !selectable;

            // Update viewport layout
            const bool juxtapose = subjectIndex == 0 && mtd.hasFlag(MessageTypeFlags::hasSecondItem);
            const auto& size = juxtapose ? layout.halfSize : layout.fullSize;

            viewportWidget.left = layout.position.x + 2;
            viewportWidget.right = layout.position.x + size.width - 4;
            buttonWidget.left = layout.position.x;
            buttonWidget.right = layout.position.x + size.width;

            // Update viewport focus
            if (_nState.savedView[subjectIndex] != view)
            {
                _nState.savedView[subjectIndex] = view;
                self.viewportRemove(subjectIndex);
                self.invalidate();

                if (!view.isEmpty())
                {
                    auto origin = viewportWidget.position() + Point{ 1, 1 };

                    uint16_t viewportWidth = viewportWidget.width();
                    uint16_t viewportHeight = 62;
                    Ui::Size viewportSize = { viewportWidth, viewportHeight };

                    if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
                    {
                        origin = viewportWidget.position();
                        viewportWidth = viewportWidget.width() + 2;
                        viewportHeight = 64;
                        viewportSize = { viewportWidth, viewportHeight };
                    }

                    if (view.isEntityView())
                    {
                        ViewportManager::create(&self, subjectIndex, origin, viewportSize, view.zoomLevel, view.entityId);
                    }
                    else
                    {
                        ViewportManager::create(&self, subjectIndex, origin, viewportSize, view.zoomLevel, view.getPos());
                    }

                    self.invalidate();
                }
            }
        }

        // 0x00429209
        void initViewports(Window& self)
        {
            initViewport(self, 0);
            initViewport(self, 1);
        }

        // 0x0042A136
        static void drawNewsSubjectImages(Window* self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            for (auto i = 0; i < 2; ++i)
            {
                const auto itemSubject = news->itemSubjects[i];
                const auto& viewWidget = self->widgets[Common::widx::viewport1 + i];
                const SubjectType subjectType = SubjectType(static_cast<uint8_t>(static_cast<int8_t>(_nState.savedView[i].zoomLevel)));

                if (subjectType == SubjectType::companyFace && itemSubject != 0xFFFFU)
                {
                    const auto* company = CompanyManager::get(CompanyId(itemSubject));
                    const auto* competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);
                    const auto imageIndexBase = competitorObj->images[enumValue(company->ownerEmotion)];

                    const ImageId imageId(imageIndexBase + 1, company->mainColours.primary);
                    const auto x = viewWidget.midX() - 31;
                    const auto y = viewWidget.midY() - 31;
                    drawingCtx.drawImage(ZoomLevel::full, Ui::Point(x, y), imageId);

                    if (company->jailStatus != 0)
                    {
                        drawingCtx.drawImage(ZoomLevel::full, Ui::Point(x, y), ImageId(ImageIds::owner_jailed));
                    }
                }

                if (subjectType == SubjectType::vehicleImage && itemSubject != 0xFFFFU)
                {
                    const auto x = viewWidget.left;
                    const auto y = viewWidget.top;

                    if (drawingCtx.pushClip(Ui::Rect(x + 1, y + 1, viewWidget.width() - 2, viewWidget.height() - 2)))
                    {
                        drawVehicleOverview(
                            drawingCtx,
                            { viewWidget.midX(), 35 },
                            itemSubject,
                            Ui::WindowManager::getVehiclePreviewRotationFrameYaw(),
                            Ui::WindowManager::getVehiclePreviewRotationFrameRoll(),
                            CompanyManager::getControllingId());

                        drawingCtx.popClip();
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
        static void drawLateNews(Window& self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            // TODO: This should be a proper setter and we should obtain it type safe.
            self.widgets[Common::widx::frame].styleData = enumValue(Widgets::NewsPanel::Style::new_);

            self.draw(drawingCtx);

            auto tr = Gfx::TextRenderer(drawingCtx);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            const auto& mtd = getMessageTypeDescriptor(news->type);

            if (!mtd.hasFlag(MessageTypeFlags::smallerFont))
            {
                tr.setCurrentFont(Gfx::Font::large);
            }

            *buffer = ControlCodes::Colour::black;
            buffer++;

            strncpy(buffer, newsString, 511);

            int16_t x = self.width / 2;
            int16_t y = 38;
            Ui::Point origin = { x, y };

            tr.drawStringCentredWrapped(origin, 352, Colour::black, StringIds::buffer_2039);

            x = 1;
            y = 1;
            origin = { x, y };

            auto argsBuf = FormatArgumentsBuffer{};
            auto args = FormatArguments{ argsBuf };
            args.push(news->date);
            tr.drawStringLeft(origin, Colour::black, StringIds::news_date, args);

            drawNewsSubjectImages(&self, drawingCtx, news);
        }

        // 0x00429934
        static void drawMiddleNews(Window& self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            const auto& mtd = getMessageTypeDescriptor(news->type);
            if (mtd.hasFlag(MessageTypeFlags::hasFirstItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
                {
                    if (news->itemSubjects[0] != 0xFFFF)
                    {

                        auto x = self.widgets[Common::widx::viewport1].left;
                        auto y = self.widgets[Common::widx::viewport1].top;
                        auto width = self.widgets[Common::widx::viewport1].width() + 1;
                        auto height = self.widgets[Common::widx::viewport1].height() + 1;
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }

            if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
                {
                    if (news->itemSubjects[1] != 0xFFFF)
                    {
                        auto x = self.widgets[Common::widx::viewport2].left;
                        auto y = self.widgets[Common::widx::viewport2].top;
                        auto width = self.widgets[Common::widx::viewport2].width() + 1;
                        auto height = self.widgets[Common::widx::viewport2].height() + 1;
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }
        }

        // 0x004299E7
        static void drawEarlyNews(Window& self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            // TODO: This should be a proper setter and we should obtain it type safe.
            self.widgets[Common::widx::frame].styleData = enumValue(Widgets::NewsPanel::Style::old);

            self.draw(drawingCtx);

            auto tr = Gfx::TextRenderer(drawingCtx);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            const auto& mtd = getMessageTypeDescriptor(news->type);

            if (!mtd.hasFlag(MessageTypeFlags::smallerFont))
            {
                tr.setCurrentFont(Gfx::Font::large);
            }

            *buffer = ControlCodes::Colour::black;
            buffer++;

            strncpy(buffer, newsString, 511);

            int16_t x = self.width / 2;
            int16_t y = 38;
            Ui::Point origin = { x, y };

            tr.drawStringCentredWrapped(origin, 352, Colour::black, StringIds::buffer_2039);

            origin.x = 4;
            origin.y = 5;

            auto argsBuf = FormatArgumentsBuffer{};
            auto args = FormatArguments{ argsBuf };
            args.push(news->date);
            tr.drawStringLeft(origin, Colour::black, StringIds::news_date, args);

            drawNewsSubjectImages(&self, drawingCtx, news);

            x = 3;
            y = 5;
            auto width = self.width - 6;
            auto height = self.height;
            auto colour = enumValue(ExtColour::translucentBrown1);
            drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);

            x = self.widgets[Common::widx::viewport1].left;
            y = self.widgets[Common::widx::viewport1].top;
            width = self.widgets[Common::widx::viewport1].width();
            height = self.widgets[Common::widx::viewport1].height();

            colour = enumValue(ExtColour::translucentBrown1);
            drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
        }

        // 0x00429761
        static void drawStationNews(Window& self, Gfx::DrawingContext& drawingCtx, Message* news)
        {
            self.draw(drawingCtx);

            auto tr = Gfx::TextRenderer(drawingCtx);

            char* newsString = news->messageString;
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer = ControlCodes::Colour::black;
            buffer++;

            strncpy(buffer, newsString, 511);

            int16_t x = self.width / 2;
            int16_t y = 17;
            Ui::Point origin = { x, y };

            tr.drawStringCentredWrapped(origin, 338, Colour::black, StringIds::buffer_2039);

            const auto& mtd = getMessageTypeDescriptor(news->type);
            if (mtd.hasFlag(MessageTypeFlags::hasFirstItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
                {
                    if (news->itemSubjects[0] != 0xFFFF)
                    {
                        x = self.widgets[Common::widx::viewport1].left;
                        y = self.widgets[Common::widx::viewport1].top;
                        auto width = self.widgets[Common::widx::viewport1].width();
                        auto height = self.widgets[Common::widx::viewport1].height();
                        constexpr auto colour = enumValue(ExtColour::translucentGrey1);
                        drawingCtx.drawRect(x, y, width, height, colour, Gfx::RectFlags::transparent);
                    }
                }
            }

            if (mtd.hasFlag(MessageTypeFlags::hasSecondItem))
            {
                if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
                {
                    if (news->itemSubjects[1] != 0xFFFF)
                    {
                        x = self.widgets[Common::widx::viewport2].left;
                        y = self.widgets[Common::widx::viewport2].top;
                        auto width = self.widgets[Common::widx::viewport2].width();
                        auto height = self.widgets[Common::widx::viewport2].height();
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

            if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
            {
                if (calcDate(news->date).year >= 1945)
                {
                    drawLateNews(self, drawingCtx, news);

                    if (calcDate(news->date).year < 1985)
                    {
                        drawMiddleNews(self, drawingCtx, news);
                    }
                }
                else
                {
                    drawEarlyNews(self, drawingCtx, news);
                }
            }
            else
            {
                drawStationNews(self, drawingCtx, news);
            }

            if (mtd.hasFlag(MessageTypeFlags::hasFirstItem) && news->itemSubjects[0] != 0xFFFF)
            {
                auto& widget = self.widgets[Common::widx::viewport1Button];
                auto x = widget.midX();
                auto y = widget.bottom - 7;
                auto width = widget.width() - 1;
                auto point = Point(x, y);

                drawViewportString(drawingCtx, point, width, mtd.argumentTypes[0], news->itemSubjects[0]);
            }
            if (mtd.hasFlag(MessageTypeFlags::hasSecondItem) && news->itemSubjects[1] != 0xFFFF)
            {
                auto& widget = self.widgets[Common::widx::viewport2Button];
                auto x = widget.midX();
                auto y = widget.bottom - 7;
                auto width = widget.width() - 1;
                auto point = Point(x, y);

                drawViewportString(drawingCtx, point, width, mtd.argumentTypes[1], news->itemSubjects[1]);
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
}
