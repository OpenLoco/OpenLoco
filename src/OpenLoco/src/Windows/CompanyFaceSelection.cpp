#include "Audio/Audio.h"
#include "Config.h"
#include "GameCommands/Company/ChangeCompanyFace.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
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
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <optional>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::CompanyFaceSelection
{
    static loco_global<CompanyId, 0x9C68F2> _9C68F2; // Use in a game command??
    static loco_global<uint16_t, 0x112C1C1> _numberCompetitorObjects;

    static WindowType _callingWindowType;

    static constexpr Ui::Size kWindowSize = { 400, 272 };
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
    static constexpr Widget widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 398, 13 }, WidgetType::caption_24, WindowColour::primary, StringIds::company_face_selection_title),
        makeWidget({ 385, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 400, 257 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 19 }, { 188, 248 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::tooltip_company_face_selection),
        makeWidget({ 265, 23 }, { 66, 66 }, WidgetType::wt_6, WindowColour::secondary),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    static std::vector<uint32_t> _inUseCompetitors;

    // 0x00434F52
    void open(const CompanyId id, const WindowType callingWindowType)
    {
        auto* self = WindowManager::bringToFront(WindowType::companyFaceSelection, 0);

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
            self->enabledWidgets = (1 << widx::close_button);
            self->initScrollWidgets();
            _9C68F2 = id;
            self->owner = id;

            const auto* skin = ObjectManager::get<InterfaceSkinObject>();
            self->setColour(WindowColour::secondary, skin->colour_0A);

            self->rowCount = _numberCompetitorObjects;
            self->rowHover = -1;
            self->object = nullptr;
        }

        // How will we be using the selected face?
        _callingWindowType = callingWindowType;

        // Make window blocking while open
        WindowManager::setCurrentModalType(WindowType::companyFaceSelection);

        // Enumerate competitors that are in use if we are applying the selection in-game
        if (_callingWindowType == WindowType::company)
            _inUseCompetitors = CompanyManager::findAllOtherInUseCompetitors(id);
        else
            _inUseCompetitors.clear();
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
    }

    // 0x435299
    static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(&self);
                break;
        }
    }

    // 0x4352BB
    static void getScrollSize([[maybe_unused]] Window& self, [[maybe_unused]] const uint32_t scrollIndex, [[maybe_unused]] uint16_t* const scrollWidth, uint16_t* const scrollHeight)
    {
        *scrollHeight = _numberCompetitorObjects * kRowHeight;
    }

    static bool isInUseCompetitor(const uint32_t objIndex)
    {
        return std::find(_inUseCompetitors.begin(), _inUseCompetitors.end(), objIndex) != _inUseCompetitors.end();
    }

    struct ObjectRow
    {
        ObjectManager::ObjectIndexEntry object;
        int16_t rowIndex;
    };
    static ObjectRow getObjectFromSelection(const int16_t& y)
    {
        const int16_t rowIndex = y / kRowHeight;
        const auto objects = ObjectManager::getAvailableObjects(ObjectType::competitor);

        if (rowIndex < 0 || static_cast<uint16_t>(rowIndex) >= objects.size())
        {
            return { ObjectManager::ObjectIndexEntry{}, -1 };
        }

        if (isInUseCompetitor(objects[rowIndex].first))
        {
            return { ObjectManager::ObjectIndexEntry{}, -1 };
        }

        return { objects[rowIndex].second, rowIndex };
    }

    // 0x00435314
    static void scrollMouseDown(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scroll_index)
    {
        const auto objRow = getObjectFromSelection(y);

        if (objRow.rowIndex == -1)
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
            args.objHeader = *objRow.object._header;

            const auto result = GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE;
            if (result)
            {
                WindowManager::close(&self);
            }
        }
        else if (_callingWindowType == WindowType::options)
        {
            auto& config = Config::get();
            config.preferredOwnerFace = *objRow.object._header;
            Config::write();

            WindowManager::close(&self);
        }
    }

    // 0x004352C7
    static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scroll_index)
    {
        auto objRow = getObjectFromSelection(y);
        if (self.rowHover == objRow.rowIndex)
        {
            return;
        }
        self.rowHover = objRow.rowIndex;
        ObjectManager::freeTemporaryObject();

        if (objRow.rowIndex != -1)
        {
            self.object = reinterpret_cast<std::byte*>(objRow.object._header);
            ObjectManager::loadTemporaryObject(*objRow.object._header);
        }
        else
        {
            self.object = nullptr;
        }
        self.invalidate();
    }

    // 0x4352B1
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, const WidgetIndex_t)
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
            const auto l = self.x + 1 + self.widgets[widx::face_frame].left;
            const auto t = self.y + 1 + self.widgets[widx::face_frame].top;
            const auto r = self.x - 1 + self.widgets[widx::face_frame].right;
            const auto b = self.y - 1 + self.widgets[widx::face_frame].bottom;
            drawingCtx.fillRect(l, t, r, b, colour, Gfx::RectFlags::none);

            const CompetitorObject* competitor = reinterpret_cast<CompetitorObject*>(ObjectManager::getTemporaryObject());
            uint32_t img = Gfx::recolour(competitor->images[0] + 1, Colour::black);
            drawingCtx.drawImage(l, t, img);
        }

        {
            const auto x = self.x + self.widgets[widx::face_frame].midX();
            const auto y = self.y + self.widgets[widx::face_frame].bottom + 3;
            const auto width = self.width - self.widgets[widx::scrollview].right - 6;
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            *str++ = ControlCodes::windowColour2;
            auto objectPtr = self.object;
            strcpy(str, ObjectManager::ObjectIndexEntry::read(&objectPtr)._name);

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
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
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

            if (index == self.rowHover)
            {
                inlineColour = ControlCodes::windowColour2;
                drawingCtx.fillRect(0, y, self.width, y + 9, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
            }

            std::string name(object.second._name);
            name.insert(0, 1, inlineColour);

            tr.setCurrentFont(Gfx::Font::medium_bold);
            AdvancedColour stringColour = Colour::black;
            if (isInUseCompetitor(object.first))
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
