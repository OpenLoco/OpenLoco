#include "Audio/Audio.h"
#include "Config.h"
#include "GameCommands/Company/ChangeCompanyFace.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/CompetitorObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <optional>
#include <ranges>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::CompanyFaceSelection
{
    static loco_global<CompanyId, 0x9C68F2> _9C68F2; // Use in a game command??

    // Count was previously 0x112C1C1
    static std::vector<ObjectManager::ObjIndexPair> _competitorList;

    static WindowType _callingWindowType;

    static constexpr Ui::Size32 kWindowSize = { 400, 272 };
    static constexpr int16_t kRowHeight = 10;

    enum widx
    {
        frame,
        caption,
        close_button,
        panel,
        scrollview,
        face_frame
    };

    // 0x509680
    static constexpr auto widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 398, 13 }, Widgets::Caption::Style::colourText, WindowColour::primary, StringIds::company_face_selection_title),
        Widgets::ImageButton({ 385, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 400, 257 }, WindowColour::secondary),
        Widgets::ScrollView({ 4, 19 }, { 188, 248 }, WindowColour::secondary, Scrollbars::vertical, StringIds::tooltip_company_face_selection),
        Widgets::Tab({ 265, 23 }, { 66, 66 }, WindowColour::secondary)

    );

    static const WindowEventList& getEvents();

    static std::vector<uint32_t> _inUseCompetitors;

    static void populateCompetitorList()
    {
        _competitorList = ObjectManager::getAvailableObjects(ObjectType::competitor);
    }

    // 0x00434F52
    void open(const CompanyId id, const WindowType callingWindowType)
    {
        auto* self = WindowManager::bringToFront(WindowType::companyFaceSelection, 0);
        populateCompetitorList();
        if (self != nullptr)
        {
            _9C68F2 = id;
            self->owner = id;
            self->invalidate();
        }
        else
        {
            self = WindowManager::createWindow(WindowType::companyFaceSelection, kWindowSize, WindowFlags::none, getEvents());
            self->setWidgets(widgets);
            self->initScrollWidgets();
            _9C68F2 = id;
            self->owner = id;

            const auto* skin = ObjectManager::get<InterfaceSkinObject>();
            self->setColour(WindowColour::secondary, skin->windowPlayerColor);

            self->rowCount = static_cast<uint16_t>(_competitorList.size());
            self->rowHover = -1;
            self->object = nullptr;
        }

        // How will we be using the selected face?
        _callingWindowType = callingWindowType;

        // Make window blocking while open
        WindowManager::setCurrentModalType(WindowType::companyFaceSelection);

        // Enumerate competitors that are in use if we are applying the selection in-game
        if (_callingWindowType == WindowType::company)
        {
            _inUseCompetitors = CompanyManager::findAllOtherInUseCompetitors(id);
        }
        else
        {
            _inUseCompetitors.clear();
        }
    }

    // 0x004352A4
    static void onClose([[maybe_unused]] Window& self)
    {
        ObjectManager::freeTemporaryObject();
        WindowManager::setCurrentModalType(WindowType::undefined);

        if (_callingWindowType == WindowType::options)
        {
            auto& config = Config::get();
            if (config.preferredOwnerFace == kEmptyObjectHeader)
            {
                config.usePreferredOwnerFace = false;
                Config::write();
            }

            WindowManager::invalidate(WindowType::options);
        }
        _competitorList.clear();
    }

    // 0x435299
    static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(&self);
                break;
        }
    }

    // 0x4352BB
    static void getScrollSize([[maybe_unused]] Window& self, [[maybe_unused]] const uint32_t scrollIndex, [[maybe_unused]] uint32_t* const scrollWidth, uint32_t* const scrollHeight)
    {
        *scrollHeight = static_cast<uint32_t>(_competitorList.size()) * kRowHeight;
    }

    static bool isInUseCompetitor(const uint32_t objIndex)
    {
        return std::find(_inUseCompetitors.begin(), _inUseCompetitors.end(), objIndex) != _inUseCompetitors.end();
    }

    static ObjectManager::ObjIndexPair getObjectFromSelection(const int16_t y)
    {
        const int16_t rowIndex = y / kRowHeight;

        if (rowIndex < 0 || static_cast<uint16_t>(rowIndex) >= _competitorList.size())
        {
            return { ObjectManager::kNullObjectIndex, ObjectManager::ObjectIndexEntry{} };
        }

        if (isInUseCompetitor(_competitorList[rowIndex].index))
        {
            return { ObjectManager::kNullObjectIndex, ObjectManager::ObjectIndexEntry{} };
        }

        return _competitorList[rowIndex];
    }

    // 0x00435314
    static void scrollMouseDown(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scroll_index)
    {
        const auto objRow = getObjectFromSelection(y);

        if (objRow.index == ObjectManager::kNullObjectIndex)
        {
            return;
        }
        self.invalidate();

        auto mousePos = Input::getMouseLocation();
        Audio::playSound(Audio::SoundId::clickDown, mousePos.x);

        if (_callingWindowType == WindowType::company)
        {
            GameCommands::setErrorTitle(StringIds::cant_select_face);

            GameCommands::ChangeCompanyFaceArgs args{};
            args.companyId = self.owner;
            args.objHeader = objRow.object._header;

            const auto result = GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE;
            if (result)
            {
                WindowManager::close(&self);
            }
        }
        else if (_callingWindowType == WindowType::options)
        {
            auto& config = Config::get();
            config.preferredOwnerFace = objRow.object._header;
            Config::write();

            WindowManager::close(&self);
        }
    }

    // 0x004352C7
    static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scroll_index)
    {
        auto objRow = getObjectFromSelection(y);
        if (self.rowHover == objRow.index)
        {
            return;
        }
        self.rowHover = objRow.index;
        ObjectManager::freeTemporaryObject();

        if (objRow.index != ObjectManager::kNullObjectIndex)
        {
            self.object = reinterpret_cast<std::byte*>(&objRow.object._header);
            ObjectManager::loadTemporaryObject(objRow.object._header);
        }
        else
        {
            self.object = nullptr;
        }
        self.invalidate();
    }

    // 0x4352B1
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, const WidgetIndex_t, [[maybe_unused]] const WidgetId id)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x434FE8
    static void prepareDraw(Window& self)
    {
        if (_callingWindowType == WindowType::company)
        {
            self.widgets[widx::caption].text = StringIds::company_face_selection_title;

            const auto company = CompanyManager::get(self.owner);
            auto args = FormatArguments(self.widgets[widx::caption].textArgs);
            args.push(company->name);
        }
        else
        {
            self.widgets[widx::caption].text = StringIds::selectPreferredCompanyOwnerFace;
        }
    }

    // 0x435003
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        if (self.rowHover == -1)
        {
            return;
        }

        auto tr = Gfx::TextRenderer(drawingCtx);

        {
            const auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 0);
            const auto l = self.widgets[widx::face_frame].left + 1;
            const auto t = self.widgets[widx::face_frame].top + 1;
            const auto r = self.widgets[widx::face_frame].right - 1;
            const auto b = self.widgets[widx::face_frame].bottom - 1;
            drawingCtx.fillRect(l, t, r, b, colour, Gfx::RectFlags::none);

            const CompetitorObject* competitor = reinterpret_cast<CompetitorObject*>(ObjectManager::getTemporaryObject());
            uint32_t img = Gfx::recolour(competitor->images[0] + 1, Colour::black);
            drawingCtx.drawImage(l, t, img);
        }

        {
            const auto x = self.widgets[widx::face_frame].midX();
            const auto y = self.widgets[widx::face_frame].bottom + 3;
            const auto width = self.width - self.widgets[widx::scrollview].right - 6;
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            *str++ = ControlCodes::windowColour2;
            strcpy(str, ObjectManager::getObjectInIndex(self.rowHover)._name.c_str());

            tr.drawStringCentredClipped(Point(x, y), width, Colour::black, StringIds::buffer_2039);
        }

        // There was code for displaying competitor stats if window opened with none
        // playing company. But that ability is disabled from the company window.
    }

    // 0x00435152
    static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.clearSingle(Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

        auto index = 0;
        for (const auto& object : _competitorList)
        {
            const int16_t y = index * kRowHeight;
            uint8_t inlineColour = ControlCodes::Colour::black;

            if (y + kRowHeight < rt.y)
            {
                index++;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            if (object.index == self.rowHover)
            {
                inlineColour = ControlCodes::windowColour2;
                drawingCtx.fillRect(0, y, self.width, y + 9, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
            }

            // copy name as we need to modify it
            std::string name = object.object._name;
            name.insert(0, 1, inlineColour);

            tr.setCurrentFont(Gfx::Font::medium_bold);
            AdvancedColour stringColour = Colour::black;
            if (isInUseCompetitor(object.index))
            {
                tr.setCurrentFont(Gfx::Font::m1);
                stringColour = self.getColour(WindowColour::secondary).opaque().inset();
            }

            tr.drawString(Point(0, y - 1), stringColour, const_cast<char*>(name.c_str()));

            index++;
        }
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = scrollMouseDown,
        .scrollMouseOver = scrollMouseOver,
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
