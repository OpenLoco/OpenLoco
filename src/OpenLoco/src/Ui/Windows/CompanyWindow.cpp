#include "Config.h"
#include "Date.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/Company/ChangeCompanyColour.h"
#include "GameCommands/Company/ChangeLoan.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
#include "GameCommands/Company/RenameCompanyName.h"
#include "GameCommands/Company/RenameCompanyOwner.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/MapSelection.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/CompetitorObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Scenario.h"
#include "ScenarioObjective.h"
#include "SceneManager.h"
#include "Ui/Dropdown.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CheckboxWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"
#include "World/Company.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cmath>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::CompanyWindow
{
    namespace Common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_status,
            tab_details,
            tab_colour_scheme,
            tab_finances,
            tab_cargo_delivered,
            tab_challenge,
            company_select,
        };

        constexpr uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_status) | (1 << widx::tab_details) | (1 << widx::tab_colour_scheme) | (1 << widx::tab_finances) | (1 << widx::tab_cargo_delivered) | (1 << widx::tab_challenge);

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { frameWidth, 120 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_owner_and_status),
                Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_headquarters_and_details),
                Widgets::Tab({ 65, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_colour_scheme),
                Widgets::Tab({ 96, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_finances),
                Widgets::Tab({ 127, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_cargo_delivered),
                Widgets::Tab({ 158, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_challenge_for_this_game),
                Widgets::ImageButton({ 0, 14 }, { 26, 26 }, WindowColour::primary, ImageIds::null, StringIds::tooltip_select_company));
        }

        // 0x004343FC
        static void disableChallengeTab(Window* self)
        {
            self->disabledWidgets = 0;
            if (CompanyId(self->number) != CompanyManager::getControllingId())
            {
                self->disabledWidgets |= (1 << widx::tab_challenge);
            }
        }

        // 0x00431E9B
        static void enableRenameByCaption(Window* self)
        {
            if (isEditorMode() || CompanyId(self->number) == CompanyManager::getControllingId())
            {
                self->enabledWidgets |= (1 << caption);
            }
            else
            {
                self->enabledWidgets &= ~(1 << caption);
            }
        }

        // Defined at the bottom of this file.
        static void renameCompanyPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void renameCompany(Window* self, const char* input);
        static void switchCompany(Window* self, int16_t itemIndex);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void switchTabWidgets(Window* self);
        static void drawCompanySelect(const Window* const self, Gfx::DrawingContext& drawingCtx);
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx);
    }

    namespace Status
    {
        static constexpr Ui::Size32 kWindowSize = { 270, 182 };

        enum widx
        {
            unk_11 = 11,
            viewport,
            centre_on_viewport,
            face,
            change_owner_name,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(270, 182, StringIds::title_company),
            Widgets::Label({ 3, 160 }, { 242, 21 }, WindowColour::secondary, ContentAlign::Center),
            makeWidget({ 3, 44 }, { 96, 120 }, WidgetType::viewport, WindowColour::secondary, Widget::kContentUnk),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::viewportCentreButton, WindowColour::secondary, Widget::kContentNull, StringIds::move_main_view_to_show_this),
            Widgets::ImageButton({ 178, 57 }, { 66, 66 }, WindowColour::secondary, Widget::kContentNull),
            Widgets::ImageButton({ 154, 124 }, { 112, 22 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_change_owner_name)

        );

        constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Common::widx::company_select) | (1 << widx::centre_on_viewport) | (1 << widx::face) | (1 << widx::change_owner_name);

        // 0x00431EBB
        static void prepareDraw(Window& self)
        {
            Common::switchTabWidgets(&self);

            // Set company name in title.
            auto company = CompanyManager::get(CompanyId(self.number));
            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(company->name);

            self.disabledWidgets &= ~((1 << widx::centre_on_viewport) | (1 << widx::face));

            // No centring on a viewport that doesn't exist.
            if (self.viewports[0] == nullptr)
            {
                self.disabledWidgets |= (1 << widx::centre_on_viewport);
            }

            // No changing other player's faces, unless we're editing a scenario.
            if (CompanyId(self.number) != CompanyManager::getControllingId() && !isEditorMode())
            {
                self.disabledWidgets |= (1 << widx::face);
            }

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[widx::viewport].right = self.width - 119;
            self.widgets[widx::viewport].bottom = self.height - 14;

            self.widgets[widx::unk_11].top = self.height - 12;
            self.widgets[widx::unk_11].bottom = self.height - 3;
            self.widgets[widx::unk_11].right = self.width - 14;

            self.widgets[widx::change_owner_name].right = self.width - 4;
            self.widgets[widx::change_owner_name].left = self.width - 116;

            self.widgets[widx::face].right = self.width - 28;
            self.widgets[widx::face].left = self.width - 93;

            self.widgets[Common::widx::company_select].right = self.width - 3;
            self.widgets[Common::widx::company_select].left = self.width - 28;

            if (CompanyId(self.number) == CompanyManager::getControllingId())
            {
                self.widgets[widx::change_owner_name].type = WidgetType::buttonWithImage;
            }
            else
            {
                self.widgets[widx::change_owner_name].type = WidgetType::none;
            }

            self.widgets[widx::centre_on_viewport].right = self.widgets[widx::viewport].right - 1;
            self.widgets[widx::centre_on_viewport].bottom = self.widgets[widx::viewport].bottom - 1;
            self.widgets[widx::centre_on_viewport].left = self.widgets[widx::viewport].right - 24;
            self.widgets[widx::centre_on_viewport].top = self.widgets[widx::viewport].bottom - 24;

            Widget::leftAlignTabs(self, Common::widx::tab_status, Common::widx::tab_challenge);
        }

        // 0x00432055
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
            Common::drawCompanySelect(&self, drawingCtx);
            const auto company = CompanyManager::get(CompanyId(self.number));
            const auto competitor = ObjectManager::get<CompetitorObject>(company->competitorId);

            // Draw 'owner' label
            {
                auto& widget = self.widgets[widx::face];
                auto point = Point(self.x + (widget.left + widget.right) / 2, self.y + widget.top - 12);
                tr.drawStringCentred(
                    point,
                    Colour::black,
                    StringIds::window_owner);
            }

            // Draw company owner image.
            {
                const uint32_t image = Gfx::recolour(competitor->images[enumValue(company->ownerEmotion)] + 1, company->mainColours.primary);
                const uint16_t x = self.x + self.widgets[widx::face].left + 1;
                const uint16_t y = self.y + self.widgets[widx::face].top + 1;
                drawingCtx.drawImage(x, y, image);
            }

            // If the owner's been naughty, draw some jail bars over them.
            if (company->jailStatus != 0)
            {
                const uint32_t image = ImageIds::owner_jailed;
                const uint16_t x = self.x + self.widgets[widx::face].left + 1;
                const uint16_t y = self.y + self.widgets[widx::face].top + 1;
                drawingCtx.drawImage(x, y, image);
            }

            // Draw owner name
            {
                FormatArguments args{};
                args.push(company->ownerName);

                auto& widget = self.widgets[widx::change_owner_name];
                auto origin = Ui::Point(self.x + (widget.left + widget.right) / 2, self.y + widget.top + 5);
                tr.drawStringCentredWrapped(
                    origin,
                    widget.right - widget.left,
                    Colour::black,
                    StringIds::black_stringid,
                    args);
            }

            // Draw owner status
            {
                // TODO: df fix this
                // Until format arguments can allow pushing to the front we will have to call twice once for the status
                FormatArguments args{};
                StringId status = CompanyManager::getOwnerStatus(CompanyId(self.number), args);
                args = FormatArguments{};
                args.push(status);
                // and once for the args
                CompanyManager::getOwnerStatus(CompanyId(self.number), args);

                auto& widget = self.widgets[widx::unk_11];
                auto point = Point(self.x + widget.left - 1, self.y + widget.top - 1);
                tr.drawStringLeftClipped(
                    point,
                    widget.right - widget.left,
                    Colour::black,
                    StringIds::black_stringid,
                    args);
            }
        }

        // 0x00432244
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameCompanyPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_status:
                case Common::widx::tab_details:
                case Common::widx::tab_colour_scheme:
                case Common::widx::tab_finances:
                case Common::widx::tab_cargo_delivered:
                case Common::widx::tab_challenge:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::centre_on_viewport:
                    self.viewportCentreMain();
                    break;

                case widx::face:
                    CompanyFaceSelection::open(CompanyId(self.number), self.type);
                    break;

                case widx::change_owner_name:
                {
                    auto company = CompanyManager::get(CompanyId(self.number));
                    TextInput::openTextInput(&self, StringIds::title_name_owner, StringIds::prompt_enter_new_name_for_owner, company->ownerName, widgetIndex, nullptr);
                    break;
                }
            }
        }

        // 0x00432283
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Common::widx::company_select)
            {
                Dropdown::populateCompanySelect(&self, &self.widgets[widgetIndex]);
            }
        }

        // 0x0043228E
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == Common::widx::company_select)
            {
                Common::switchCompany(&self, itemIndex);
            }
        }

        // 0x004325DF
        static void renameCompanyOwnerName(Window* self, const char* input)
        {
            if (strlen(input) == 0)
            {
                return;
            }

            GameCommands::setErrorTitle(StringIds::cannot_change_owner_name);

            bool success = false;
            {
                GameCommands::ChangeCompanyOwnerNameArgs args{};

                args.companyId = CompanyId(self->number);
                args.bufferIndex = 1;
                std::memcpy(args.newName, input, 36);

                GameCommands::doCommand(args, GameCommands::Flags::apply);

                args.bufferIndex = 2;

                GameCommands::doCommand(args, GameCommands::Flags::apply);

                args.bufferIndex = 0;

                success = GameCommands::doCommand(args, GameCommands::Flags::apply);
            }

            // No need to propagate the name if it could not be set.
            if (!success)
            {
                return;
            }

            // Only name company after owner if this is a new company.
            const auto& company = CompanyManager::get(CompanyId(self->number));
            if (company->name != StringIds::new_company)
            {
                return;
            }

            // Temporarily store the new name in buffer string 2039.
            // TODO: replace with a fixed length!
            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            strcpy(buffer, input);

            FormatArguments args{};
            args.push(StringIds::buffer_2039);
            // Add the ' Transport' suffix to the company name, and rename the company.
            StringManager::formatString(buffer, StringIds::company_owner_name_transport, args);
            Common::renameCompany(self, buffer);
        }

        // 0x004322F6
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget == Common::widx::caption)
            {
                Common::renameCompany(&self, input);
            }
            else if (callingWidget == widx::change_owner_name)
            {
                renameCompanyOwnerName(&self, input);
            }
        }

        // 0x0043270A
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self.number);
        }

        // 0x00432724
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(Status::kWindowSize, { 640, 400 });

            if (self.viewports[0] != nullptr)
            {
                Ui::Size proposedDims(self.width - 123, self.height - 59);
                auto& viewport = self.viewports[0];
                if (proposedDims.width != viewport->width || proposedDims.height != viewport->height)
                {
                    viewport->width = proposedDims.width;
                    viewport->height = proposedDims.height;
                    viewport->viewWidth = proposedDims.width << viewport->zoom;
                    viewport->viewHeight = proposedDims.height << viewport->zoom;
                    self.savedView.clear();
                }
            }

            self.callViewportRotate();
        }

        static void sub_434336(Window* self, const SavedView& view)
        {
            if (self->viewports[0] != nullptr)
            {
                return;
            }

            auto& widget = self->widgets[widx::viewport];
            auto origin = Ui::Point(widget.left + self->x + 1, widget.top + self->y + 1);
            auto size = Ui::Size(widget.width() - 2, widget.height() - 2);
            if (view.isEntityView())
            {
                ViewportManager::create(self, 0, origin, size, self->savedView.zoomLevel, view.entityId);
            }
            else
            {
                ViewportManager::create(self, 0, origin, size, self->savedView.zoomLevel, view.getPos());
            }
        }

        static void sub_434223(Window* const self, const SavedView& view, const ViewportFlags vpFlags)
        {
            self->savedView = view;
            sub_434336(self, view);
            self->viewports[0]->flags |= vpFlags;
            self->invalidate();
        }

        static void differentViewportSettings(Window* const self, const SavedView& view)
        {
            auto vpFlags = self->viewports[0]->flags;
            self->viewportRemove(0);
            sub_434223(self, view, vpFlags);
        }

        static void noViewportPresent(Window* const self, const SavedView& view)
        {
            ViewportFlags vpFlags = ViewportFlags::none;
            if (Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
            {
                vpFlags |= ViewportFlags::gridlines_on_landscape;
            }
            sub_434223(self, view, vpFlags);
        }

        static void invalidViewport(Window* const self)
        {
            self->viewportRemove(0);
            self->invalidate();
        }

        // 0x004327C8
        static void viewportRotate(Window& self)
        {
            if (self.currentTab != 0)
            {
                return;
            }

            self.callPrepareDraw();

            const auto& company = CompanyManager::get(CompanyId(self.number));

            if (company->observationEntity == EntityId::null)
            {
                // Observing a certain location?
                if (company->observationX != -1)
                {
                    auto tileZAndWater = World::TileManager::getHeight({ company->observationX, company->observationY });
                    coord_t tileZ = tileZAndWater.landHeight;
                    coord_t waterZ = tileZAndWater.waterHeight;
                    if (waterZ != 0)
                    {
                        tileZ = waterZ;
                    }

                    // loc_43410A
                    int8_t rotation = static_cast<int8_t>(self.viewports[0]->getRotation());
                    SavedView view(
                        company->observationX,
                        company->observationY,
                        ZoomLevel::half,
                        rotation,
                        static_cast<int16_t>(tileZ + 16));
                    view.flags |= (1 << 14);

                    if (self.viewports[0] == nullptr)
                    {
                        noViewportPresent(&self, view);
                        return;
                    }

                    if (self.savedView.isEntityView() || self.savedView.rotation != view.rotation || self.savedView.zoomLevel != view.zoomLevel)
                    {
                        if (self.savedView != view)
                        {
                            differentViewportSettings(&self, view);
                            return;
                        }
                        return;
                    }

                    self.savedView = view;
                    self.viewportCentreOnTile(view.getPos());
                    return;
                }
                // Not observing anything at all?
                else
                {
                    invalidViewport(&self);
                }
            }
            else
            {
                // loc_434170
                auto entity = EntityManager::get<OpenLoco::EntityBase>(company->observationEntity);
                auto* vehicle = entity->asBase<Vehicles::VehicleBase>();
                if (vehicle == nullptr)
                {
                    invalidViewport(&self);
                    return;
                }
                if (!vehicle->isVehicleHead() || (vehicle->position.x == Location::null))
                {
                    invalidViewport(&self);
                    return;
                }

                Vehicles::Vehicle train(vehicle->getHead());

                int8_t rotation = static_cast<int8_t>(self.viewports[0]->getRotation());
                SavedView view(
                    train.cars.firstCar.body->id,
                    0xC000,
                    ZoomLevel::full,
                    rotation,
                    0);

                if (self.viewports[0] == nullptr)
                {
                    noViewportPresent(&self, view);
                    return;
                }

                if (self.savedView != view)
                {
                    differentViewportSettings(&self, view);
                    return;
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .textInput = textInput,
            .viewportRotate = viewportRotate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x004347D0
    static Window* create(CompanyId companyId)
    {
        const WindowFlags newFlags = WindowFlags::flag_8 | WindowFlags::flag_11;
        auto window = WindowManager::createWindow(WindowType::company, Status::kWindowSize, newFlags, Status::getEvents());
        window->number = enumValue(companyId);
        window->owner = companyId;
        window->currentTab = 0;
        window->frameNo = 0;
        window->savedView.clear();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::secondary, skin->colour_0A);

        window->flags |= WindowFlags::resizable;

        return window;
    }

    // 0x0043454F
    Window* open(CompanyId companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
            {
                ToolManager::toolCancel();
                window = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
            }
        }

        if (window == nullptr)
        {
            window = create(companyId);
        }

        window->currentTab = 0;
        window->width = Status::kWindowSize.width;
        window->height = Status::kWindowSize.height;
        window->invalidate();

        window->setWidgets(Status::widgets);
        window->enabledWidgets = Status::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Status::getEvents();
        window->activatedWidgets = 0;

        Common::disableChallengeTab(window);
        window->initScrollWidgets();
        window->moveInsideScreenEdges();

        return window;
    }

    // 0x00435ACC
    Window* openAndSetName()
    {
        CompanyId companyId = CompanyManager::getControllingId();
        Window* self = open(companyId);

        // Allow setting company owner name if no preferred owner name has been set.
        if (!Config::get().usePreferredOwnerName)
        {
            Status::onMouseUp(*self, Status::widx::change_owner_name);
        }

        return self;
    }

    namespace Details
    {
        static constexpr Ui::Size32 kWindowSize = { 340, 194 };

        loco_global<World::Pos3, 0x009C68D6> _headquarterGhostPos;
        loco_global<uint8_t, 0x009C68F0> _headquarterGhostRotation;
        loco_global<uint8_t, 0x009C68F1> _headquarterGhostType;
        loco_global<bool, 0x009C68EF> _headquarterGhostPlaced;

        // New in OpenLoco; not to be confused with rotation of already-placed HQ ghost
        static uint8_t _headquarterConstructionRotation;

        enum widx
        {
            viewport = 11,
            build_hq,
            centre_on_viewport,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(340, 194, StringIds::title_company_details),
            makeWidget({ 219, 54 }, { 96, 120 }, WidgetType::viewport, WindowColour::secondary, Widget::kContentUnk),
            Widgets::ImageButton({ 315, 92 }, { 24, 24 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_build_or_move_headquarters),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::viewportCentreButton, WindowColour::secondary, Widget::kContentNull, StringIds::move_main_view_to_show_this)

        );

        constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Common::widx::company_select) | (1 << build_hq) | (1 << centre_on_viewport);

        // 0x004327CF
        static void prepareDraw(Window& self)
        {
            Common::switchTabWidgets(&self);

            // Set company name.
            auto company = CompanyManager::get(CompanyId(self.number));
            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(company->name);

            auto companyColour = CompanyManager::getCompanyColour(CompanyId(self.number));
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            uint32_t image = skin->img + InterfaceSkin::ImageIds::build_headquarters;
            self.widgets[widx::build_hq].image = Gfx::recolour(image, companyColour) | Widget::kImageIdColourSet;

            self.disabledWidgets &= ~(1 << widx::centre_on_viewport);
            if (company->headquartersX == -1)
            {
                self.disabledWidgets |= (1 << widx::centre_on_viewport);
            }

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[widx::viewport].right = self.width - 26;
            self.widgets[widx::viewport].bottom = self.height - 14;

            self.widgets[Common::widx::company_select].right = self.width - 3;
            self.widgets[Common::widx::company_select].left = self.width - 28;

            if (CompanyId(self.number) == CompanyManager::getControllingId())
            {
                self.widgets[widx::build_hq].type = WidgetType::buttonWithImage;
            }
            else
            {
                self.widgets[widx::build_hq].type = WidgetType::none;
            }

            self.widgets[widx::centre_on_viewport].right = self.widgets[widx::viewport].right - 1;
            self.widgets[widx::centre_on_viewport].bottom = self.widgets[widx::viewport].bottom - 1;
            self.widgets[widx::centre_on_viewport].left = self.widgets[widx::viewport].right - 24;
            self.widgets[widx::centre_on_viewport].top = self.widgets[widx::viewport].bottom - 24;

            Widget::leftAlignTabs(self, Common::widx::tab_status, Common::widx::tab_challenge);
        }

        static void drawAIdetails(Gfx::DrawingContext& drawingCtx, const int32_t x, int32_t& y, const OpenLoco::Company& company)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            const auto competitor = ObjectManager::get<CompetitorObject>(company.competitorId);
            {
                FormatArguments args{};
                args.push<uint16_t>(competitor->intelligence);
                args.push(aiRatingToLevel(competitor->intelligence));

                auto point = Point(x, y);
                tr.drawStringLeft(point, Colour::black, StringIds::company_details_intelligence, args);
                y += 10;
            }
            {
                FormatArguments args{};
                args.push<uint16_t>(competitor->aggressiveness);
                args.push(aiRatingToLevel(competitor->aggressiveness));

                auto point = Point(x, y);
                tr.drawStringLeft(point, Colour::black, StringIds::company_details_aggressiveness, args);
                y += 10;
            }
            {
                FormatArguments args{};
                args.push<uint16_t>(competitor->competitiveness);
                args.push(aiRatingToLevel(competitor->competitiveness));

                auto point = Point(x, y);
                tr.drawStringLeft(point, Colour::black, StringIds::company_details_competitiveness, args);
                y += 10;
            }
        }

        static std::array<StringId, 6> transportTypeCountString = {
            {
                StringIds::company_details_trains_count,
                StringIds::company_details_buses_count,
                StringIds::company_details_trucks_count,
                StringIds::company_details_trams_count,
                StringIds::company_details_aircraft_count,
                StringIds::company_details_ships_count,
            }
        };

        // 0x00432919
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
            Common::drawCompanySelect(&self, drawingCtx);

            auto company = CompanyManager::get(CompanyId(self.number));
            auto x = self.x + 3;
            auto y = self.y + 48;
            {
                FormatArguments args{};
                args.push(company->startedDate);

                auto point = Point(x, y);
                tr.drawStringLeft(point, Colour::black, StringIds::company_details_started, args);
                y += 10;
            }

            {
                FormatArguments args{};
                formatPerformanceIndex(company->performanceIndex, args);

                StringId formatId = StringIds::company_details_performance;
                if ((company->challengeFlags & CompanyFlags::decreasedPerformance) != CompanyFlags::none)
                {
                    formatId = StringIds::company_details_performance_decreasing;
                }
                else if ((company->challengeFlags & CompanyFlags::increasedPerformance) != CompanyFlags::none)
                {
                    formatId = StringIds::company_details_performance_increasing;
                }

                auto point = Point(x, y);
                tr.drawStringLeft(point, Colour::black, formatId, args);
                y += 25;
            }

            {
                FormatArguments args{};
                args.push(company->ownerName);

                auto point = Point(x, y);
                tr.drawStringLeftClipped(point, 213, Colour::black, StringIds::owner_label, args);
                y += 10;
            }

            if (!CompanyManager::isPlayerCompany(CompanyId(self.number)))
            {
                drawAIdetails(drawingCtx, x + 5, y, *company);
            }
            y += 5;

            {
                for (auto i = 0; i < 6; ++i)
                {
                    auto count = company->transportTypeCount[i];
                    if (count != 0)
                    {
                        FormatArguments args{};
                        args.push(count);

                        auto point = Point(x, y);
                        tr.drawStringLeft(point, Colour::black, transportTypeCountString[i], args);
                        y += 10;
                    }
                }
            }

            {
                auto& widget = self.widgets[widx::viewport];
                auto point = Point(self.x + widget.midX(), self.y + widget.top - 12);
                tr.drawStringCentred(point, Colour::black, StringIds::wcolour2_headquarters);
            }

            if (company->headquartersX == -1)
            {
                auto& widget = self.widgets[widx::viewport];
                auto loc = Point(self.x + widget.midX(), self.y + widget.midY() - 5);
                auto width = widget.width() - 2;
                tr.drawStringCentredWrapped(loc, width, Colour::black, StringIds::not_yet_constructed);
            }
        }

        // 0x00432BDD
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameCompanyPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_status:
                case Common::widx::tab_details:
                case Common::widx::tab_colour_scheme:
                case Common::widx::tab_finances:
                case Common::widx::tab_cargo_delivered:
                case Common::widx::tab_challenge:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::centre_on_viewport:
                    self.viewportCentreMain();
                    break;
            }
        }

        // 0x00432C08
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::company_select:
                    Dropdown::populateCompanySelect(&self, &self.widgets[widgetIndex]);
                    break;

                case widx::build_hq:
                    ToolManager::toolSet(&self, widgetIndex, CursorId::placeHQ);
                    Input::setFlag(Input::Flags::flag6);
                    break;
            }
        }

        // 0x00432C19
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == Common::widx::company_select)
            {
                Common::switchCompany(&self, itemIndex);
            }
        }

        static void onTabSwitch()
        {
            _headquarterConstructionRotation = (WindowManager::getCurrentRotation() + 2) & 3;
        }

        static void rotateHQGhost90Deg()
        {
            _headquarterConstructionRotation = (_headquarterConstructionRotation + 1) & 3;
        }

        // 0x00432C24
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget == Common::widx::caption)
            {
                Common::renameCompany(&self, input);
            }
        }

        // 0x00434E94
        static void removeHeadquarterGhost()
        {
            if (_headquarterGhostPlaced)
            {
                _headquarterGhostPlaced = false;
                auto flags = GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost;
                GameCommands::HeadquarterRemovalArgs args;
                args.pos = _headquarterGhostPos;
                GameCommands::doCommand(args, flags);
            }
        }

        // 0x00434E3F
        static void placeHeadquarterGhost(const GameCommands::HeadquarterPlacementArgs& args)
        {
            removeHeadquarterGhost();
            auto flags = GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost;
            if (GameCommands::doCommand(args, flags) != GameCommands::FAILURE)
            {
                _headquarterGhostPlaced = true;
                _headquarterGhostPos = args.pos;
                _headquarterGhostRotation = args.rotation;
                _headquarterGhostType = args.type;
            }
        }

        // 0x00434EC7
        // input:
        // regs.ax = mouseX;
        // regs.bx = mouseY;
        // output (not verified):
        // regs.cx = tileX (tile coordinate)
        // regs.ax = tileY (tile coordinate)
        // regs.di = tileZ (height)
        // regs.bh = rotation and buildImmediately
        // regs.dx = dx - company index (value 1 in testing case)
        static std::optional<GameCommands::HeadquarterPlacementArgs> getHeadquarterPlacementArgsFromCursor(const int16_t mouseX, const int16_t mouseY)
        {
            auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ mouseX, mouseY });
            if (!pos)
            {
                return {};
            }

            GameCommands::HeadquarterPlacementArgs args;
            args.type = CompanyManager::getHeadquarterBuildingType();
            args.rotation = _headquarterConstructionRotation;

            auto tile = World::TileManager::get(*pos);
            const auto* surface = tile.surface();
            if (surface == nullptr)
            {
                return {};
            }

            auto z = surface->baseHeight(); // di
            if (surface->slope())
            {
                z += 16;
            }
            args.pos = World::Pos3(pos->x, pos->y, z);
            if (isEditorMode())
            {
                args.buildImmediately = true; // bh
            }
            return { args };
        }

        // 0x00432CA1
        static void onToolUpdate([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
            auto placementArgs = getHeadquarterPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeHeadquarterGhost();
                return;
            }

            // Always show buildings, not scaffolding, for ghost placements.
            placementArgs->buildImmediately = true;

            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            World::setMapSelectionCorner(MapSelectionType::full);

            // TODO: This selection may be incorrect if getHeadquarterBuildingType returns 0
            auto posB = World::Pos2(placementArgs->pos) + World::Pos2(32, 32);
            World::setMapSelectionArea(placementArgs->pos, posB);
            World::mapInvalidateSelectionRect();

            if (_headquarterGhostPlaced)
            {
                if (*_headquarterGhostPos == placementArgs->pos && _headquarterGhostRotation == placementArgs->rotation && _headquarterGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeHeadquarterGhost();
            placeHeadquarterGhost(*placementArgs);
        }

        // 0x00432D45
        // regs.esi = window* w;
        // regs.dx = widgetIndex;
        // regs.ax = mouseX;
        // regs.bx = mouseY;
        static void onToolDown([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t mouseX, const int16_t mouseY)
        {
            removeHeadquarterGhost();

            auto placementArgs = getHeadquarterPlacementArgsFromCursor(mouseX, mouseY);
            if (!placementArgs)
            {
                return;
            }

            GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);
            uint8_t flags = GameCommands::Flags::apply | GameCommands::Flags::flag_1;
            auto commandResult = GameCommands::doCommand(*placementArgs, flags);
            if (commandResult != GameCommands::FAILURE)
            {
                ToolManager::toolCancel();
            }
        }

        // 0x00432D7A
        static void onToolAbort([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex)
        {
            removeHeadquarterGhost();
            Ui::Windows::Main::hideGridlines();
        }

        static void onClose(Window& self)
        {
            if (ToolManager::isToolActive(self.type, self.number))
            {
                ToolManager::toolCancel();
            }
        }

        // 0x0432D85
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self.number);
        }

        // 0x00432D9F
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);
            self.setSize(kWindowSize);
            self.callViewportRotate();
        }

        static void sub_434377(Window* self, const SavedView& view)
        {
            if (self->viewports[0] != nullptr)
            {
                return;
            }

            auto& widget = self->widgets[widx::viewport];
            auto origin = Ui::Point(widget.left + self->x + 1, widget.top + self->y + 1);
            auto size = Ui::Size(widget.width() - 2, widget.height() - 2);

            ViewportManager::create(self, 0, origin, size, self->savedView.zoomLevel, view.getPos());
            self->flags |= WindowFlags::viewportNoScrolling;
            self->invalidate();
        }

        // 0x00432E08
        static void viewportRotate(Window& self)
        {
            if (self.currentTab != Common::tab_details - Common::tab_status)
            {
                return;
            }

            self.callPrepareDraw();
            auto company = CompanyManager::get(CompanyId(self.number));
            if (company->headquartersX == -1)
            {
                // If headquarters not placed destroy the viewport
                self.viewportRemove(0);
                self.invalidate();
                return;
            }
            int8_t rotation = static_cast<int8_t>(self.viewports[0]->getRotation());
            World::Pos3 loc = {
                static_cast<coord_t>(company->headquartersX + 32),
                static_cast<coord_t>(company->headquartersY + 32),
                static_cast<coord_t>((company->headquartersZ + 8) * World::kSmallZStep)
            };
            SavedView view{
                loc.x,
                loc.y,
                ZoomLevel::full,
                rotation,
                loc.z
            };
            view.flags |= (1 << 14);

            ViewportFlags vpFlags = ViewportFlags::none;
            if (self.viewports[0] == nullptr)
            {
                if (Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
                {
                    vpFlags |= ViewportFlags::gridlines_on_landscape;
                }
            }
            else if (self.savedView != view)
            {
                vpFlags = self.viewports[0]->flags;
                self.viewportRemove(0);
            }
            else
            {
                return;
            }

            self.savedView = view;
            sub_434377(&self, view);
            if (self.viewports[0] != nullptr)
            {
                self.viewports[0]->flags = vpFlags;
                self.invalidate();
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .onToolAbort = onToolAbort,
            .textInput = textInput,
            .viewportRotate = viewportRotate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    bool rotate(Window& self)
    {
        if (self.currentTab != Common::widx::tab_details - Common::widx::tab_status)
        {
            return false;
        }

        Details::rotateHQGhost90Deg();
        return true;
    }

    namespace ColourScheme
    {
        static constexpr Ui::Size32 kWindowSize = { 265, 252 };

        enum widx
        {
            check_steam_locomotives = 11,
            check_diesel_locomotives,
            check_electric_locomotives,
            check_multiple_units,
            check_passenger_vehicles,
            check_freight_vehicles,
            check_buses,
            check_trucks,
            check_aircraft,
            check_ships,
            main_colour_scheme,
            main_colour_steam_locomotives,
            main_colour_diesel_locomotives,
            main_colour_electric_locomotives,
            main_colour_multiple_units,
            main_colour_passenger_vehicles,
            main_colour_freight_vehicles,
            main_colour_buses,
            main_colour_trucks,
            main_colour_aircraft,
            main_colour_ships,
            secondary_colour_scheme,
            secondary_colour_steam_locomotives,
            secondary_colour_diesel_locomotives,
            secondary_colour_electric_locomotives,
            secondary_colour_multiple_units,
            secondary_colour_passenger_vehicles,
            secondary_colour_freight_vehicles,
            secondary_colour_buses,
            secondary_colour_trucks,
            secondary_colour_aircraft,
            secondary_colour_ships,
        };

        // clang-format off
        constexpr uint64_t allMainColours = {
            (1ULL << widx::main_colour_scheme) |
            (1ULL << widx::main_colour_steam_locomotives) |
            (1ULL << widx::main_colour_diesel_locomotives) |
            (1ULL << widx::main_colour_electric_locomotives) |
            (1ULL << widx::main_colour_multiple_units) |
            (1ULL << widx::main_colour_passenger_vehicles) |
            (1ULL << widx::main_colour_freight_vehicles) |
            (1ULL << widx::main_colour_buses) |
            (1ULL << widx::main_colour_trucks) |
            (1ULL << widx::main_colour_aircraft) |
            (1ULL << widx::main_colour_ships)
        };

        constexpr uint64_t allSecondaryColours = {
            (1ULL << widx::secondary_colour_scheme) |
            (1ULL << widx::secondary_colour_steam_locomotives) |
            (1ULL << widx::secondary_colour_diesel_locomotives) |
            (1ULL << widx::secondary_colour_electric_locomotives) |
            (1ULL << widx::secondary_colour_multiple_units) |
            (1ULL << widx::secondary_colour_passenger_vehicles) |
            (1ULL << widx::secondary_colour_freight_vehicles) |
            (1ULL << widx::secondary_colour_buses) |
            (1ULL << widx::secondary_colour_trucks) |
            (1ULL << widx::secondary_colour_aircraft) |
            (1ULL << widx::secondary_colour_ships)
        };

        constexpr uint64_t allColourChecks = {
            (1ULL << widx::check_steam_locomotives) |
            (1ULL << widx::check_diesel_locomotives) |
            (1ULL << widx::check_electric_locomotives) |
            (1ULL << widx::check_multiple_units) |
            (1ULL << widx::check_passenger_vehicles) |
            (1ULL << widx::check_freight_vehicles) |
            (1ULL << widx::check_buses) |
            (1ULL << widx::check_trucks) |
            (1ULL << widx::check_aircraft) |
            (1ULL << widx::check_ships)
        };
        // clang-format on

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(265, 252, StringIds::title_company_colour_scheme),
            Widgets::Checkbox({ 15, 81 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_steam_locomotives, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 98 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_diesel_locomotives, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 115 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_electric_locomotives, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 132 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_multiple_units, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 149 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_passenger_vehicles, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 166 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_freight_vehicles, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 183 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_buses, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 200 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_trucks, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 217 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_aircraft, StringIds::tooltip_toggle_vehicle_colour_scheme),
            Widgets::Checkbox({ 15, 234 }, { 204, 12 }, WindowColour::secondary, StringIds::colour_ships, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 221, 48 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 78 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 95 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 112 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 129 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 146 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 163 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 180 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 197 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 214 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 231 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_main_colour),
            makeWidget({ 239, 48 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 78 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 95 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 112 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 129 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 146 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 163 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 180 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 197 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 214 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 231 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_secondary_colour)

        );

        constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Common::widx::company_select) | allMainColours | allSecondaryColours | allColourChecks;

        // 0x00432E0F
        static void prepareDraw(Window& self)
        {
            Common::switchTabWidgets(&self);

            // Set company name.
            auto company = CompanyManager::get(CompanyId(self.number));
            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(company->name);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[Common::widx::company_select].right = self.width - 3;
            self.widgets[Common::widx::company_select].left = self.width - 28;

            Widget::leftAlignTabs(self, Common::widx::tab_status, Common::widx::tab_challenge);

            // Set company's main colour
            self.widgets[widx::main_colour_scheme].image = Widget::kImageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->mainColours.primary);

            // Set company's secondary colour
            self.widgets[widx::secondary_colour_scheme].image = Widget::kImageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->mainColours.secondary);

            struct ColourSchemeTuple
            {
                WidgetIndex_t checkbox;
                WidgetIndex_t primary;
                WidgetIndex_t secondary;
            };

            // clang-format off
            static constexpr ColourSchemeTuple tuples[] =
            {
                { widx::check_steam_locomotives,     widx::main_colour_steam_locomotives,     widx::secondary_colour_steam_locomotives },
                { widx::check_diesel_locomotives,    widx::main_colour_diesel_locomotives,    widx::secondary_colour_diesel_locomotives },
                { widx::check_electric_locomotives,  widx::main_colour_electric_locomotives,  widx::secondary_colour_electric_locomotives },
                { widx::check_multiple_units,        widx::main_colour_multiple_units,        widx::secondary_colour_multiple_units },
                { widx::check_passenger_vehicles,    widx::main_colour_passenger_vehicles,    widx::secondary_colour_passenger_vehicles },
                { widx::check_freight_vehicles,      widx::main_colour_freight_vehicles,      widx::secondary_colour_freight_vehicles },
                { widx::check_buses,                 widx::main_colour_buses,                 widx::secondary_colour_buses },
                { widx::check_trucks,                widx::main_colour_trucks,                widx::secondary_colour_trucks },
                { widx::check_aircraft,              widx::main_colour_aircraft,              widx::secondary_colour_aircraft },
                { widx::check_ships,                 widx::main_colour_ships,                 widx::secondary_colour_ships },
            };
            // clang-format on

            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(tuples)); i++)
            {
                // customVehicleColoursSet appears to reserve its first bit for something else, so skip it.
                if ((company->customVehicleColoursSet & (1 << (i + 1))) != 0)
                {
                    self.activatedWidgets |= (1ULL << tuples[i].checkbox);

                    self.widgets[tuples[i].primary].image = (1ULL << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->vehicleColours[i].primary);
                    self.widgets[tuples[i].secondary].image = (1ULL << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->vehicleColours[i].secondary);

                    self.widgets[tuples[i].primary].type = WidgetType::buttonWithColour;
                    self.widgets[tuples[i].secondary].type = WidgetType::buttonWithColour;
                }
                else
                {
                    self.activatedWidgets &= ~(1ULL << tuples[i].checkbox);

                    self.widgets[tuples[i].primary].type = WidgetType::none;
                    self.widgets[tuples[i].secondary].type = WidgetType::none;
                }
            }

            if (CompanyId(self.number) == CompanyManager::getControllingId())
            {
                self.enabledWidgets |= allColourChecks | allMainColours | allSecondaryColours;
            }
            else
            {
                self.enabledWidgets &= ~(allColourChecks | allMainColours | allSecondaryColours);
            }
        }

        // 0x00432F9A
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
            Common::drawCompanySelect(&self, drawingCtx);

            const auto& widget = self.widgets[widx::main_colour_scheme];
            auto point = Point(self.x + 6, self.y + widget.top + 3);

            // 'Main colour scheme'
            tr.drawStringLeft(
                point,
                Colour::black,
                StringIds::main_colour_scheme);

            // 'Special colour schemes used for'
            point.y += 17;
            tr.drawStringLeft(
                point,
                Colour::black,
                StringIds::special_colour_schemes_used_for);
        }

        // 0x00433032
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameCompanyPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_status:
                case Common::widx::tab_details:
                case Common::widx::tab_colour_scheme:
                case Common::widx::tab_finances:
                case Common::widx::tab_cargo_delivered:
                case Common::widx::tab_challenge:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::check_steam_locomotives:
                case widx::check_diesel_locomotives:
                case widx::check_electric_locomotives:
                case widx::check_multiple_units:
                case widx::check_passenger_vehicles:
                case widx::check_freight_vehicles:
                case widx::check_buses:
                case widx::check_trucks:
                case widx::check_aircraft:
                case widx::check_ships:
                    // customVehicleColoursSet reserves first bit for main colour scheme even though it can't be changed, so skip it.
                    const auto vehicleType = widgetIndex - widx::check_steam_locomotives + 1;
                    const auto company = CompanyManager::get(CompanyId(self.number));
                    const auto newMode = (company->customVehicleColoursSet & (1 << vehicleType)) == 0 ? 1 : 0;

                    GameCommands::setErrorTitle(StringIds::error_cant_change_colour_scheme);

                    GameCommands::ChangeCompanyColourSchemeArgs args{};

                    args.value = newMode;
                    args.colourType = vehicleType;
                    args.setColourMode = 1;
                    args.companyId = CompanyId(self.number);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);

                    break;
            }
        }

        // 0x00433067
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::company_select:
                    Dropdown::populateCompanySelect(&self, &self.widgets[widgetIndex]);
                    break;

                case main_colour_scheme:
                case main_colour_steam_locomotives:
                case main_colour_diesel_locomotives:
                case main_colour_electric_locomotives:
                case main_colour_multiple_units:
                case main_colour_passenger_vehicles:
                case main_colour_freight_vehicles:
                case main_colour_buses:
                case main_colour_trucks:
                case main_colour_aircraft:
                case main_colour_ships:
                {
                    auto* company = CompanyManager::get(CompanyId(self.number));
                    Colour selectedColour;
                    if (widgetIndex != main_colour_scheme)
                    {
                        auto vehicleType = widgetIndex - main_colour_steam_locomotives;
                        selectedColour = company->vehicleColours[vehicleType].primary;
                    }
                    else
                    {
                        selectedColour = company->mainColours.primary;
                    }

                    auto availableColours = 0x7FFFFFFF & ~(CompanyManager::competingColourMask(CompanyId(self.number)));
                    Dropdown::showColour(&self, &self.widgets[widgetIndex], availableColours, selectedColour, self.getColour(WindowColour::secondary));
                    break;
                }

                case secondary_colour_scheme:
                case secondary_colour_steam_locomotives:
                case secondary_colour_diesel_locomotives:
                case secondary_colour_electric_locomotives:
                case secondary_colour_multiple_units:
                case secondary_colour_passenger_vehicles:
                case secondary_colour_freight_vehicles:
                case secondary_colour_buses:
                case secondary_colour_trucks:
                case secondary_colour_aircraft:
                case secondary_colour_ships:
                {
                    auto* company = CompanyManager::get(CompanyId(self.number));
                    Colour selectedColour;
                    if (widgetIndex != secondary_colour_scheme)
                    {
                        auto vehicleType = widgetIndex - secondary_colour_steam_locomotives;
                        selectedColour = company->vehicleColours[vehicleType].secondary;
                    }
                    else
                    {
                        selectedColour = company->mainColours.secondary;
                    }

                    auto availableColours = 0x7FFFFFFF;
                    Dropdown::showColour(&self, &self.widgets[widgetIndex], availableColours, selectedColour, self.getColour(WindowColour::secondary));
                    break;
                }
            }
        }

        // 0x00433092
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget == Common::widx::caption)
            {
                Common::renameCompany(&self, input);
            }
        }

        // 0x0043309D
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::company_select:
                    Common::switchCompany(&self, itemIndex);
                    break;

                case widx::main_colour_scheme:
                case widx::main_colour_steam_locomotives:
                case widx::main_colour_diesel_locomotives:
                case widx::main_colour_electric_locomotives:
                case widx::main_colour_multiple_units:
                case widx::main_colour_passenger_vehicles:
                case widx::main_colour_freight_vehicles:
                case widx::main_colour_buses:
                case widx::main_colour_trucks:
                case widx::main_colour_aircraft:
                case widx::main_colour_ships:
                {
                    if (itemIndex == -1)
                    {
                        return;
                    }

                    GameCommands::setErrorTitle(StringIds::error_cant_change_colour_scheme);

                    const int8_t colour = Dropdown::getItemArgument(itemIndex, 2);
                    const auto vehicleType = widgetIndex - widx::main_colour_scheme;

                    GameCommands::ChangeCompanyColourSchemeArgs args{};

                    args.isPrimary = false;
                    args.value = colour;
                    args.colourType = vehicleType;
                    args.setColourMode = 0;
                    args.companyId = CompanyId(self.number);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);

                    break;
                }

                case widx::secondary_colour_scheme:
                case widx::secondary_colour_steam_locomotives:
                case widx::secondary_colour_diesel_locomotives:
                case widx::secondary_colour_electric_locomotives:
                case widx::secondary_colour_multiple_units:
                case widx::secondary_colour_passenger_vehicles:
                case widx::secondary_colour_freight_vehicles:
                case widx::secondary_colour_buses:
                case widx::secondary_colour_trucks:
                case widx::secondary_colour_aircraft:
                case widx::secondary_colour_ships:
                {
                    if (itemIndex == -1)
                    {
                        return;
                    }

                    GameCommands::setErrorTitle(StringIds::error_cant_change_colour_scheme);

                    const int8_t colour = Dropdown::getItemArgument(itemIndex, 2);
                    const auto vehicleType = widgetIndex - widx::secondary_colour_scheme;

                    GameCommands::ChangeCompanyColourSchemeArgs args{};

                    args.isPrimary = true;
                    args.value = colour;
                    args.colourType = vehicleType;
                    args.setColourMode = 0;
                    args.companyId = CompanyId(self.number);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
        }

        // 0x0043325F
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self.number);
        }

        // 0x00433279
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);
            self.setSize(kWindowSize);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .textInput = textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Finances
    {
        static constexpr Ui::Size32 kWindowSize = { 636, 319 };

        enum widx
        {
            scrollview = 11,
            currentLoan,
            loan_decrease,
            loan_increase,
            loan_autopay,
        };

        constexpr uint16_t expenditureColumnWidth = 128;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(636, 319, StringIds::title_company_finances),
            makeWidget({ 133, 45 }, { 499, 215 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::horizontal),
            makeStepperWidgets({ 87, 264 }, { 100, 12 }, WindowColour::secondary, StringIds::company_current_loan_value),
            Widgets::Checkbox({ 320, 264 }, { 204, 12 }, WindowColour::secondary, StringIds::loan_autopay, StringIds::tooltip_loan_autopay) // loan_autopay

        );

        constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Common::widx::company_select) | (1 << widx::loan_decrease) | (1 << widx::loan_increase) | (1 << widx::loan_autopay);

        const uint64_t holdableWidgets = (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        // 0x004332E4
        static void prepareDraw(Window& self)
        {
            Common::switchTabWidgets(&self);

            auto company = CompanyManager::get(CompanyId(self.number));

            // Set company name.
            {
                auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
                args.push(company->name);
            }

            // Set current loan value.
            {
                auto args = FormatArguments(self.widgets[widx::currentLoan].textArgs);
                args.push(company->currentLoan);
            }

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[Common::widx::company_select].right = self.width - 3;
            self.widgets[Common::widx::company_select].left = self.width - 28;

            if (company->id() == CompanyManager::getControllingId())
            {
                self.widgets[widx::currentLoan].type = WidgetType::textbox;
                self.widgets[widx::loan_decrease].type = WidgetType::button;
                self.widgets[widx::loan_increase].type = WidgetType::button;
                self.widgets[widx::loan_autopay].type = WidgetType::checkbox;

                if ((company->challengeFlags & CompanyFlags::autopayLoan) != CompanyFlags::none)
                {
                    self.activatedWidgets |= (1ULL << Finances::widx::loan_autopay);
                }
                else
                {
                    self.activatedWidgets &= ~(1ULL << Finances::widx::loan_autopay);
                }
            }
            else
            {
                self.widgets[widx::currentLoan].type = WidgetType::none;
                self.widgets[widx::loan_decrease].type = WidgetType::none;
                self.widgets[widx::loan_increase].type = WidgetType::none;
                self.widgets[widx::loan_autopay].type = WidgetType::none;
            }

            Widget::leftAlignTabs(self, Common::widx::tab_status, Common::widx::tab_challenge);
        }

        // 0x004333D0
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
            Common::drawCompanySelect(&self, drawingCtx);

            const auto company = CompanyManager::get(CompanyId(self.number));

            // Draw 'expenditure/income' label
            {
                auto point = Point(self.x + 5, self.y + 47);
                tr.drawStringLeftUnderline(
                    point,
                    Colour::black,
                    StringIds::expenditure_income);
            }

            const StringId ExpenditureLabels[] = {
                StringIds::train_income,
                StringIds::train_running_costs,
                StringIds::bus_income,
                StringIds::bus_running_costs,
                StringIds::truck_income,
                StringIds::truck_running_costs,
                StringIds::tram_income,
                StringIds::tram_running_costs,
                StringIds::aircraft_income,
                StringIds::aircraft_running_costs,
                StringIds::ship_income,
                StringIds::ship_running_costs,
                StringIds::construction,
                StringIds::vehicle_purchases,
                StringIds::vehicle_disposals,
                StringIds::loan_interest,
                StringIds::miscellaneous,
            };

            uint16_t y = self.y + 62;
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(ExpenditureLabels)); i++)
            {
                // Add zebra stripes to even labels.
                if (i % 2 == 0)
                {
                    auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 6);
                    drawingCtx.fillRect(self.x + 4, y, self.x + 129, y + 9, colour, Gfx::RectFlags::crossHatching);
                }

                FormatArguments args{};
                args.push(ExpenditureLabels[i]);

                auto point = Point(self.x + 5, y - 1);
                tr.drawStringLeft(
                    point,
                    Colour::black,
                    StringIds::wcolour2_stringid,
                    args);

                y += 10;
            }

            // 'Current loan' label
            {
                auto point = Point(self.x + 7, self.y + self.widgets[widx::currentLoan].top);
                tr.drawStringLeft(
                    point,
                    Colour::black,
                    StringIds::company_current_loan);
            }

            // '@ X% interest per' label
            {
                FormatArguments args{};
                args.push<uint16_t>(getGameState().loanInterestRate);

                auto& widget = self.widgets[widx::currentLoan];
                auto point = Point(self.x + widget.right + 3, self.y + widget.top + 1);
                tr.drawStringLeft(
                    point,
                    Colour::black,
                    StringIds::interest_per_year,
                    args);
            }

            // 'Cash' label with value
            {
                // Set cash value in format args.
                FormatArguments args{};
                args.push(company->cash);

                auto cashFormat = StringIds::cash_positive;
                if ((company->challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
                {
                    cashFormat = StringIds::cash_bankrupt;
                }
                if (company->cash.var_04 < 0)
                {
                    cashFormat = StringIds::cash_negative;
                }

                auto point = Point(self.x + 7, self.y + self.widgets[widx::currentLoan].top + 13);
                tr.drawStringLeft(
                    point,
                    Colour::black,
                    cashFormat,
                    args);
            }

            // 'Company value' label with value
            {
                // Set company value in format args.
                FormatArguments args{};
                args.push(company->companyValueHistory[0]);

                auto point = Point(self.x + 7, self.y + self.widgets[widx::currentLoan].top + 26);
                tr.drawStringLeft(
                    point,
                    Colour::black,
                    StringIds::company_value,
                    args);
            }

            // 'Profit from vehicles' label with value
            {
                // Set company value in format args.
                FormatArguments args{};
                args.push(company->vehicleProfit);

                auto point = Point(self.x + 7, self.y + self.widgets[widx::currentLoan].top + 39);
                tr.drawStringLeft(
                    point,
                    Colour::black,
                    StringIds::profit_from_vehicles,
                    args);
            }
        }

        static void drawFinanceYear(Gfx::DrawingContext& drawingCtx, int16_t x, int16_t& y, uint16_t columnYear, uint16_t currentYear)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            FormatArguments args{};
            args.push(StringIds::uint16_raw);
            args.push(columnYear);

            StringId format = StringIds::wcolour2_stringid;
            if (columnYear != currentYear)
            {
                format = StringIds::black_stringid;
            }

            auto point = Point(x, y);
            tr.drawStringRightUnderline(
                point,
                Colour::black,
                format,
                args);

            y += 14;
        }

        static currency48_t drawFinanceExpenditureColumn(Gfx::DrawingContext& drawingCtx, const int16_t x, int16_t& y, uint8_t columnIndex, Company& company)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            currency48_t sum = 0;
            for (auto j = 0; j < ExpenditureType::Count; j++)
            {
                currency48_t expenditures = company.expenditures[columnIndex][j];
                sum += expenditures;

                if (expenditures != 0)
                {
                    FormatArguments args{};
                    args.push(StringIds::currency48);
                    args.push(expenditures);

                    auto point = Point(x, y);
                    tr.drawStringRight(
                        point,
                        Colour::black,
                        StringIds::black_stringid,
                        args);
                }
                y += 10;
            }
            return sum;
        }

        static void drawFinanceSum(Gfx::DrawingContext& drawingCtx, int16_t x, int16_t& y, currency48_t sum)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            auto mainFormat = StringIds::black_stringid;
            auto sumFormat = StringIds::plus_currency48;
            if (sum < 0)
            {
                mainFormat = StringIds::red_stringid;
                sumFormat = StringIds::currency48;
            }

            FormatArguments args{};
            args.push(sumFormat);
            args.push(sum);

            y += 4;

            auto point = Point(x, y);
            tr.drawStringRight(point, Colour::black, mainFormat, args);

            drawingCtx.fillRect(x - expenditureColumnWidth + 10, y - 2, x, y - 2, PaletteIndex::black0, Gfx::RectFlags::none);
        }

        // 0x0043361E
        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            int16_t y = 47 - self.widgets[widx::scrollview].top + 14;

            for (uint8_t i = 0; i < static_cast<uint8_t>(ExpenditureType::Count); i++)
            {
                // Add zebra stripes to even labels.
                if (i % 2 == 0)
                {
                    auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 6);
                    drawingCtx.fillRect(0, y, expenditureColumnWidth * 17, y + 9, colour, Gfx::RectFlags::crossHatching);
                }

                y += 10;
            }

            const auto company = CompanyManager::get(CompanyId(self.number));

            uint32_t curYear = getCurrentYear();
            uint8_t expenditureYears = std::min<uint8_t>(company->numExpenditureYears, kExpenditureHistoryCapacity);

            // Paint years on top of scroll area.
            int16_t x = 132 - self.widgets[widx::scrollview].left + expenditureColumnWidth;
            for (auto i = 0; i < expenditureYears; i++)
            {
                y = 46 - self.widgets[widx::scrollview].top;

                uint16_t columnYear = curYear - (expenditureYears - i) + 1;
                uint8_t columnIndex = expenditureYears - i - 1;
                drawFinanceYear(drawingCtx, x, y, columnYear, curYear);
                auto sum = drawFinanceExpenditureColumn(drawingCtx, x, y, columnIndex, *company);
                drawFinanceSum(drawingCtx, x, y, sum);

                x += expenditureColumnWidth;
            }
        }

        // 0x00433819
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameCompanyPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_status:
                case Common::widx::tab_details:
                case Common::widx::tab_colour_scheme:
                case Common::widx::tab_finances:
                case Common::widx::tab_cargo_delivered:
                case Common::widx::tab_challenge:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::loan_autopay:
                {
                    auto company = CompanyManager::get(CompanyId(self.number));
                    company->challengeFlags ^= CompanyFlags::autopayLoan;
                    break;
                }
            }
        }

        static inline currency32_t calculateStepSize(uint16_t repeatTicks)
        {
            return 1000 * std::pow(10, repeatTicks / 100);
        }

        // 0x0043383E
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::company_select:
                    Dropdown::populateCompanySelect(&self, &self.widgets[widgetIndex]);
                    break;

                case widx::loan_decrease:
                {
                    auto company = CompanyManager::get(CompanyId(self.number));
                    if (company->currentLoan == 0)
                    {
                        return;
                    }

                    GameCommands::ChangeLoanArgs args{};
                    args.newLoan = std::max<currency32_t>(0, company->currentLoan - calculateStepSize(Input::getClickRepeatTicks()));

                    GameCommands::setErrorTitle(StringIds::cant_pay_back_loan);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }

                case widx::loan_increase:
                {
                    GameCommands::ChangeLoanArgs args{};
                    args.newLoan = CompanyManager::get(CompanyId(self.number))->currentLoan + calculateStepSize(Input::getClickRepeatTicks());

                    GameCommands::setErrorTitle(StringIds::cant_borrow_any_more_money);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
        }

        // 0x0043385D
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget == Common::widx::caption)
            {
                Common::renameCompany(&self, input);
            }
        }

        // 0x004C8DBF
        // For the finances the most recent data on the scroll view is the data
        // most off to the right of the scroll. This function moves to the last
        // page of data as far to the right of the scroll.
        static void scrollToLatestData(Window* self)
        {
            self->initScrollWidgets();
            self->scrollAreas[0].contentOffsetX = 0x7FFF;
            self->scrollAreas[0].contentWidth = 0;
            self->updateScrollWidgets();

            const Ui::Widget& widget = self->widgets[widx::scrollview];

            const auto x = std::max<int16_t>(0, self->scrollAreas[0].contentOffsetX);
            auto widgetWidth = widget.width() - 2;
            if (self->scrollAreas[0].hasFlags(ScrollFlags::vscrollbarVisible))
            {
                widgetWidth -= ScrollView::barWidth;
            }
            // This gets the offset of the last full page (widgetWidth) of the scroll view
            const auto newOffset = std::max(0, self->scrollAreas[0].contentWidth - widgetWidth);
            self->scrollAreas[0].contentOffsetX = std::min<int16_t>(x, newOffset);
            ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x00433868
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == Common::widx::company_select)
            {
                Common::switchCompany(&self, itemIndex);
                scrollToLatestData(&self);
                self.invalidate();
            }
        }

        // 0x0043386F
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, uint16_t* scrollWidth, [[maybe_unused]] uint16_t* scrollHeight)
        {
            const auto& company = CompanyManager::get(CompanyId(self.number));
            *scrollWidth = company->numExpenditureYears * expenditureColumnWidth;
        }

        // 0x00433887
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_list);
            return args;
        }

        // 0x0043399D
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self.number);
        }

        // 0x004339B7
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);
            self.setSize(kWindowSize);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .getScrollSize = getScrollSize,
            .textInput = textInput,
            .tooltip = tooltip,
            .prepareDraw = prepareDraw,
            .draw = draw,
            .drawScroll = drawScroll,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x004345EE
    Window* openFinances(CompanyId companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
            {
                ToolManager::toolCancel();
                window = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
            }
        }

        if (window == nullptr)
        {
            window = create(companyId);
        }

        window->currentTab = Common::tab_finances - Common::tab_status;
        window->width = Finances::kWindowSize.width;
        window->height = Finances::kWindowSize.height;
        window->invalidate();

        window->setWidgets(Finances::widgets);
        window->enabledWidgets = Finances::enabledWidgets;
        window->holdableWidgets = Finances::holdableWidgets;
        window->eventHandlers = &Finances::getEvents();
        window->activatedWidgets = 0;

        Common::disableChallengeTab(window);
        window->initScrollWidgets();
        window->moveInsideScreenEdges();
        Finances::scrollToLatestData(window);

        return window;
    }

    namespace CargoDelivered
    {
        static constexpr Ui::Size32 kWindowSize = { 240, 382 };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(240, 382, StringIds::title_company_cargo_delivered)

        );

        constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Common::widx::company_select);

        // 0x00433A22
        static void prepareDraw(Window& self)
        {
            Common::switchTabWidgets(&self);

            // Set company name.
            auto company = CompanyManager::get(CompanyId(self.number));
            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(company->name);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            Widget::leftAlignTabs(self, Common::widx::tab_status, Common::widx::tab_challenge);
        }

        // 0x00433ACD
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            uint16_t y = self.y + 47;

            // 'Cargo delivered'
            {
                auto point = Point(self.x + 5, y);
                tr.drawStringLeft(point, Colour::black, StringIds::cargo_delivered);
            }

            y += 10;

            uint8_t numPrinted = 0;
            const auto company = CompanyManager::get(CompanyId(self.number));
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(company->cargoDelivered)); i++)
            {
                auto cargo = ObjectManager::get<CargoObject>(i);
                if (cargo == nullptr || company->cargoDelivered[i] == 0)
                {
                    continue;
                }

                FormatArguments args{};
                if (company->cargoDelivered[i] == 1)
                {
                    args.push(cargo->unitNameSingular);
                }
                else
                {
                    args.push(cargo->unitNamePlural);
                }

                args.push(company->cargoDelivered[i]);

                auto point = Point(self.x + 10, y);
                tr.drawStringLeft(point, Colour::black, StringIds::black_stringid, args);

                numPrinted++;
                y += 10;
            }

            // No cargo delivered yet?
            if (numPrinted == 0)
            {
                auto point = Point(self.x + 10, y);
                tr.drawStringLeft(point, Colour::black, StringIds::cargo_delivered_none);
            }
        }

        // 0x00433BE6
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameCompanyPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_status:
                case Common::widx::tab_details:
                case Common::widx::tab_colour_scheme:
                case Common::widx::tab_finances:
                case Common::widx::tab_cargo_delivered:
                case Common::widx::tab_challenge:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x00433C0B
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Common::widx::company_select)
            {
                Dropdown::populateCompanySelect(&self, &self.widgets[widgetIndex]);
            }
        }

        // 0x00433C16
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget == Common::widx::caption)
            {
                Common::renameCompany(&self, input);
            }
        }

        // 0x00433C21
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == Common::widx::company_select)
            {
                Common::switchCompany(&self, itemIndex);
            }
        }

        // 0x00433C7D
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self.number);
        }

        // 0x00433C97
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            uint16_t cargoHeight = 0;
            const auto company = CompanyManager::get(CompanyId(self.number));
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(company->cargoDelivered)); i++)
            {
                auto cargo = ObjectManager::get<CargoObject>(i);
                if (cargo == nullptr || company->cargoDelivered[i] == 0)
                {
                    continue;
                }

                cargoHeight += 10;
            }

            const uint16_t kWindowHeight = std::max<int16_t>(cargoHeight, 50) + 62;

            self.setSize({ kWindowSize.width, kWindowHeight });
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .textInput = textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Challenge
    {
        static constexpr Ui::Size32 kWindowSize = { 320, 182 };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(320, 182, StringIds::title_company_challenge)

        );

        constexpr uint64_t enabledWidgets = Common::enabledWidgets;

        // 0x00433D39
        static void prepareDraw(Window& self)
        {
            Common::switchTabWidgets(&self);

            // Set company name.
            auto company = CompanyManager::get(CompanyId(self.number));
            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(company->name);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[Common::widx::company_select].right = self.width - 3;
            self.widgets[Common::widx::company_select].left = self.width - 28;
            self.widgets[Common::widx::company_select].type = WidgetType::none;

            Widget::leftAlignTabs(self, Common::widx::tab_status, Common::widx::tab_challenge);
        }

        // 0x00433DEB
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            *buffer_2039++ = static_cast<char>(ControlCodes::Colour::black);
            char* scenarioDetailsString = getGameState().scenarioDetails;
            StringManager::locoStrcpy(buffer_2039, scenarioDetailsString);

            auto point = Point(self.x + 5, self.y + 47);

            // for example: "Provide the transport services on this little island" for "Boulder Breakers" scenario
            point = tr.drawStringLeftWrapped(point, self.width - 10, Colour::black, StringIds::buffer_2039);
            point.y += 5;

            tr.drawStringLeft(point, Colour::black, StringIds::challenge_label);
            point.y += 10;

            {
                FormatArguments args{};
                Scenario::formatChallengeArguments(Scenario::getObjective(), Scenario::getObjectiveProgress(), args);

                point = tr.drawStringLeftWrapped(point, self.width - 10, Colour::black, StringIds::challenge_value, args);
                point.y += 5;
            }

            Company* playerCompany = CompanyManager::getPlayerCompany();

            if ((playerCompany->challengeFlags & CompanyFlags::challengeCompleted) != CompanyFlags::none)
            {
                uint16_t years = Scenario::getObjectiveProgress().completedChallengeInMonths / 12;
                uint16_t months = Scenario::getObjectiveProgress().completedChallengeInMonths % 12;

                FormatArguments args{};
                args.push(years);
                args.push(months);

                tr.drawStringLeftWrapped(point, self.width - 10, Colour::black, StringIds::success_you_completed_the_challenge_in_years_months, args);
                return;
            }

            if ((playerCompany->challengeFlags & CompanyFlags::challengeFailed) != CompanyFlags::none)
            {
                tr.drawStringLeftWrapped(point, self.width - 10, Colour::black, StringIds::failed_you_failed_to_complete_the_challenge);
                return;
            }

            if ((playerCompany->challengeFlags & CompanyFlags::challengeBeatenByOpponent) != CompanyFlags::none)
            {
                uint16_t years = Scenario::getObjectiveProgress().completedChallengeInMonths / 12;
                uint16_t months = Scenario::getObjectiveProgress().completedChallengeInMonths % 12;

                FormatArguments args{};
                args.push(CompanyManager::getOpponent()->ownerName);
                args.skip(2);
                args.push(years);
                args.push(months);
                tr.drawStringLeftWrapped(point, self.width - 10, Colour::black, StringIds::beaten_by_other_player_completed_in_years_months, args);
                return;
            }

            {
                FormatArguments args{};
                args.push<uint16_t>(playerCompany->challengeProgress);
                point = tr.drawStringLeftWrapped(point, self.width - 10, Colour::black, StringIds::progress_towards_completing_challenge_percent, args);
            }

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::withinTimeLimit) != Scenario::ObjectiveFlags::none)
            {
                // time limited challenge
                uint16_t monthsLeft = Scenario::getObjective().timeLimitYears * 12 - Scenario::getObjectiveProgress().monthsInChallenge;
                uint16_t years = monthsLeft / 12;
                uint16_t months = monthsLeft % 12;

                FormatArguments args{};
                args.push(years);
                args.push(months);

                tr.drawStringLeftWrapped(point, self.width + 10, Colour::black, StringIds::time_remaining_years_months, args);
                return;
            }
        }

        // 0x00433FFE
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameCompanyPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_status:
                case Common::widx::tab_details:
                case Common::widx::tab_colour_scheme:
                case Common::widx::tab_finances:
                case Common::widx::tab_cargo_delivered:
                case Common::widx::tab_challenge:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x00434023
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget == Common::widx::caption)
            {
                Common::renameCompany(&self, input);
            }
        }

        // 0x0043402E
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self.number);
        }

        // 0x00434048
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = onUpdate,
            .textInput = textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 00434731
    Window* openChallenge(CompanyId companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
            {
                ToolManager::toolCancel();
                window = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
            }
        }

        if (window == nullptr)
        {
            window = create(companyId);
        }

        window->currentTab = Common::tab_challenge - Common::tab_status;
        window->width = Challenge::kWindowSize.width;
        window->height = Challenge::kWindowSize.height;
        window->invalidate();

        window->setWidgets(Challenge::widgets);
        window->enabledWidgets = Challenge::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Challenge::getEvents();
        window->activatedWidgets = 0;

        Common::disableChallengeTab(window);
        window->initScrollWidgets();
        window->moveInsideScreenEdges();

        return window;
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t* enabledWidgets;
            const Ui::Size32* kWindowSize;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { Status::widgets,         widx::tab_status,          Status::getEvents(),         &Status::enabledWidgets,         &Status::kWindowSize },
            { Details::widgets,        widx::tab_details,         Details::getEvents(),        &Details::enabledWidgets,        &Details::kWindowSize },
            { ColourScheme::widgets,   widx::tab_colour_scheme,   ColourScheme::getEvents(),   &ColourScheme::enabledWidgets,   &ColourScheme::kWindowSize },
            { Finances::widgets,       widx::tab_finances,        Finances::getEvents(),       &Finances::enabledWidgets,       &Finances::kWindowSize },
            { CargoDelivered::widgets, widx::tab_cargo_delivered, CargoDelivered::getEvents(), &CargoDelivered::enabledWidgets, &CargoDelivered::kWindowSize },
            { Challenge::widgets,      widx::tab_challenge,       Challenge::getEvents(),      &Challenge::enabledWidgets,      &Challenge::kWindowSize }
        };
        // clang-format on

        static void switchCompany(Window* self, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                return;
            }

            CompanyId companyId = Dropdown::getCompanyIdFromSelection(itemIndex);

            // Try to find an open company window for this company.
            auto companyWindow = WindowManager::bringToFront(WindowType::company, enumValue(companyId));
            if (companyWindow != nullptr)
            {
                return;
            }

            // If not, we'll turn this window into a window for the company selected.
            auto company = CompanyManager::get(companyId);
            if (company->name == StringIds::empty)
            {
                return;
            }

            self->number = enumValue(companyId);
            self->owner = companyId;

            Common::disableChallengeTab(self);
            self->invalidate();
        }

        static void switchTabWidgets(Window* self)
        {
            self->activatedWidgets = 0;

            static std::span<const Widget> widgetCollectionsByTabId[] = {
                Status::widgets,
                Details::widgets,
                ColourScheme::widgets,
                Finances::widgets,
                CargoDelivered::widgets,
                Challenge::widgets,
            };

            auto newWidgets = widgetCollectionsByTabId[self->currentTab];
            self->setWidgets(newWidgets);
            // self->initScrollWidgets();

            static constexpr widx tabWidgetIdxByTabId[] = {
                tab_status,
                tab_details,
                tab_colour_scheme,
                tab_finances,
                tab_cargo_delivered,
                tab_challenge,
            };

            self->activatedWidgets &= ~((1 << tab_status) | (1 << tab_details) | (1 << tab_colour_scheme) | (1 << tab_finances) | (1 << tab_cargo_delivered) | (1 << tab_challenge));
            self->activatedWidgets |= (1ULL << tabWidgetIdxByTabId[self->currentTab]);
        }

        // 0x0043230B
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self->type, self->number))
            {
                ToolManager::toolCancel();
            }

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_status;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            auto tabIndex = widgetIndex - widx::tab_status;
            auto tabInfo = tabInformationByTabOffset[tabIndex];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = &tabInfo.events;
            self->activatedWidgets = 0;
            self->setWidgets(tabInfo.widgets);

            if (tabInfo.widgetIndex == widx::tab_finances)
            {
                self->holdableWidgets = Finances::holdableWidgets;
            }

            Common::disableChallengeTab(self);
            self->invalidate();
            self->setSize(*tabInfo.kWindowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();

            if (tabInfo.widgetIndex == widx::tab_details)
            {
                Details::onTabSwitch();
            }

            if (tabInfo.widgetIndex == widx::tab_finances)
            {
                Finances::scrollToLatestData(self);
            }
        }

        // 0x0043252E
        static void renameCompanyPrompt(Window* self, WidgetIndex_t widgetIndex)
        {
            auto company = CompanyManager::get(CompanyId(self->number));
            TextInput::openTextInput(self, StringIds::title_name_company, StringIds::prompt_enter_new_company_name, company->name, widgetIndex, nullptr);
        }

        // 0x0043254F
        static void renameCompany(Window* self, const char* input)
        {
            if (strlen(input) == 0)
            {
                return;
            }

            GameCommands::setErrorTitle(StringIds::cannot_rename_this_company);

            GameCommands::ChangeCompanyNameArgs args{};

            args.companyId = CompanyId(self->number);
            args.bufferIndex = 1;
            std::memcpy(args.buffer, input, 36);

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.bufferIndex = 2;

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.bufferIndex = 0;

            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        static void drawCompanySelect(const Window* const self, Gfx::DrawingContext& drawingCtx)
        {
            const auto company = CompanyManager::get(CompanyId(self->number));
            const auto competitor = ObjectManager::get<CompetitorObject>(company->competitorId);

            // Draw company owner face.
            const uint32_t image = Gfx::recolour(competitor->images[enumValue(company->ownerEmotion)], company->mainColours.primary);
            const uint16_t x = self->x + self->widgets[Common::widx::company_select].left + 1;
            const uint16_t y = self->y + self->widgets[Common::widx::company_select].top + 1;
            drawingCtx.drawImage(x, y, image);
        }

        // 0x00434413
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Status tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_company;
                Widget::drawTab(self, drawingCtx, imageId, widx::tab_status);
            }

            // Details tab
            {
                const uint32_t imageId = Gfx::recolour(skin->img + InterfaceSkin::ImageIds::tab_company_details, self->getColour(WindowColour::primary).c());
                Widget::drawTab(self, drawingCtx, imageId, widx::tab_details);
            }

            // Colour scheme tab
            {
                static constexpr uint32_t colourSchemeTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame0,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame1,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame2,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame3,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame4,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame5,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame6,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_colour_scheme - widx::tab_status)
                {
                    imageId += colourSchemeTabImageIds[(self->frameNo / 4) % std::size(colourSchemeTabImageIds)];
                }
                else
                {
                    imageId += colourSchemeTabImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_colour_scheme);
            }

            // Finances tab
            {
                static constexpr uint32_t financesTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_finances_frame0,
                    InterfaceSkin::ImageIds::tab_finances_frame1,
                    InterfaceSkin::ImageIds::tab_finances_frame2,
                    InterfaceSkin::ImageIds::tab_finances_frame3,
                    InterfaceSkin::ImageIds::tab_finances_frame4,
                    InterfaceSkin::ImageIds::tab_finances_frame5,
                    InterfaceSkin::ImageIds::tab_finances_frame6,
                    InterfaceSkin::ImageIds::tab_finances_frame7,
                    InterfaceSkin::ImageIds::tab_finances_frame8,
                    InterfaceSkin::ImageIds::tab_finances_frame9,
                    InterfaceSkin::ImageIds::tab_finances_frame10,
                    InterfaceSkin::ImageIds::tab_finances_frame11,
                    InterfaceSkin::ImageIds::tab_finances_frame12,
                    InterfaceSkin::ImageIds::tab_finances_frame13,
                    InterfaceSkin::ImageIds::tab_finances_frame14,
                    InterfaceSkin::ImageIds::tab_finances_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_finances - widx::tab_status)
                {
                    imageId += financesTabImageIds[(self->frameNo / 2) % std::size(financesTabImageIds)];
                }
                else
                {
                    imageId += financesTabImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_finances);
            }

            // Cargo delivered tab
            {
                static constexpr uint32_t cargoDeliveredTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame0,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame1,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame2,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame3,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_cargo_delivered - widx::tab_status)
                {
                    imageId += cargoDeliveredTabImageIds[(self->frameNo / 4) % std::size(cargoDeliveredTabImageIds)];
                }
                else
                {
                    imageId += cargoDeliveredTabImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo_delivered);
            }

            // Challenge tab
            {
                static constexpr uint32_t challengeTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cup_frame0,
                    InterfaceSkin::ImageIds::tab_cup_frame1,
                    InterfaceSkin::ImageIds::tab_cup_frame2,
                    InterfaceSkin::ImageIds::tab_cup_frame3,
                    InterfaceSkin::ImageIds::tab_cup_frame4,
                    InterfaceSkin::ImageIds::tab_cup_frame5,
                    InterfaceSkin::ImageIds::tab_cup_frame6,
                    InterfaceSkin::ImageIds::tab_cup_frame7,
                    InterfaceSkin::ImageIds::tab_cup_frame8,
                    InterfaceSkin::ImageIds::tab_cup_frame9,
                    InterfaceSkin::ImageIds::tab_cup_frame10,
                    InterfaceSkin::ImageIds::tab_cup_frame11,
                    InterfaceSkin::ImageIds::tab_cup_frame12,
                    InterfaceSkin::ImageIds::tab_cup_frame13,
                    InterfaceSkin::ImageIds::tab_cup_frame14,
                    InterfaceSkin::ImageIds::tab_cup_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_challenge - widx::tab_status)
                {
                    imageId += challengeTabImageIds[(self->frameNo / 4) % std::size(challengeTabImageIds)];
                }
                else
                {
                    imageId += challengeTabImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_challenge);
            }
        }
    }
}
