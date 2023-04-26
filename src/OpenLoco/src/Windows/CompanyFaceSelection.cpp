#include "Audio/Audio.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/CompetitorObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <optional>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::CompanyFaceSelection
{
    static loco_global<CompanyId, 0x9C68F2> _9C68F2; // Use in a game command??
    static loco_global<uint16_t, 0x112C1C1> _numberCompetitorObjects;
    static loco_global<int32_t, 0x0113E72C> _cursorX;

    static constexpr Ui::Size kWindowSize = { 400, 272 };
    static constexpr int16_t kRowHeight = 10;
    static WindowEventList events;

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
    static Widget widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 398, 13 }, WidgetType::caption_24, WindowColour::primary, StringIds::company_face_selection_title),
        makeWidget({ 385, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 400, 257 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 19 }, { 188, 248 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::tooltip_company_face_selection),
        makeWidget({ 265, 23 }, { 66, 66 }, WidgetType::wt_6, WindowColour::secondary),
        widgetEnd(),
    };

    static void initEvents();

    static std::vector<uint32_t> _inUseCompetitors;

    // 0x004353F4
    static void findAllInUseCompetitors(const CompanyId id)
    {
        std::vector<uint8_t> takenCompetitorIds;
        for (const auto& c : CompanyManager::companies())
        {
            if (c.id() != id)
            {
                takenCompetitorIds.push_back(c.competitorId);
            }
        }

        _inUseCompetitors.clear();
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
        {
            auto competitorId = ObjectManager::findObjectHandle(*object.second._header);
            if (competitorId)
            {
                auto res = std::find(takenCompetitorIds.begin(), takenCompetitorIds.end(), competitorId->id);
                if (res != takenCompetitorIds.end())
                {
                    _inUseCompetitors.push_back(object.first);
                }
            }
        }
    }

    // 0x00434F52
    void open(const CompanyId id)
    {
        auto* self = WindowManager::bringToFront(WindowType::companyFaceSelection, 0);

        if (self)
        {
            _9C68F2 = id;
            self->owner = id;
            findAllInUseCompetitors(id);
            self->invalidate();
        }
        else
        {
            initEvents();
            self = WindowManager::createWindow(WindowType::companyFaceSelection, kWindowSize, WindowFlags::none, &events);
            self->widgets = widgets;
            self->enabledWidgets = (1 << widx::close_button);
            self->initScrollWidgets();
            _9C68F2 = id;
            self->owner = id;
            const auto* skin = ObjectManager::get<InterfaceSkinObject>();
            self->setColour(WindowColour::secondary, skin->colour_0A);
            findAllInUseCompetitors(id);
            self->rowCount = _numberCompetitorObjects;
            self->rowHover = -1;
            self->object = nullptr;
        }
    }

    // 0x004352A4
    static void onClose([[maybe_unused]] Window& self)
    {
        ObjectManager::freeTemporaryObject();
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
        Audio::playSound(Audio::SoundId::clickDown, _cursorX);
        GameCommands::setErrorTitle(StringIds::cant_select_face);
        const auto result = GameCommands::do_65(*objRow.object._header, self.owner);
        if (result)
        {
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
        const auto company = CompanyManager::get(self.owner);

        FormatArguments args{};
        args.push(company->name);
    }

    // 0x435003
    static void draw(Window& self, Gfx::RenderTarget* const rt)
    {
        self.draw(rt);
        if (self.rowHover == -1)
        {
            return;
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        {
            const auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 0);
            const auto l = self.x + 1 + self.widgets[widx::face_frame].left;
            const auto t = self.y + 1 + self.widgets[widx::face_frame].top;
            const auto r = self.x - 1 + self.widgets[widx::face_frame].right;
            const auto b = self.y - 1 + self.widgets[widx::face_frame].bottom;
            drawingCtx.fillRect(*rt, l, t, r, b, colour, Drawing::RectFlags::none);

            const CompetitorObject* competitor = reinterpret_cast<CompetitorObject*>(ObjectManager::getTemporaryObject());
            uint32_t img = competitor->images[0] + 1 + (1 << 29);
            drawingCtx.drawImage(rt, l, t, img);
        }

        {
            const auto x = self.x + self.widgets[widx::face_frame].midX();
            const auto y = self.y + self.widgets[widx::face_frame].bottom + 3;
            const auto width = self.width - self.widgets[widx::scrollview].right - 6;
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            *str++ = ControlCodes::windowColour2;
            auto objectPtr = self.object;
            strcpy(str, ObjectManager::ObjectIndexEntry::read(&objectPtr)._name);
            drawingCtx.drawStringCentredClipped(*rt, x, y, width, Colour::black, StringIds::buffer_2039);
        }

        // There was code for displaying competitor stats if window opened with none
        // playing company. But that ability is disabled from the company window.
    }

    // 0x00435152
    static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.clearSingle(rt, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

        auto index = 0;
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
        {
            const int16_t y = index * kRowHeight;
            uint8_t inlineColour = ControlCodes::Colour::black;

            if (y + kRowHeight < rt.y || y > rt.y + rt.height)
            {
                index++;
                continue;
            }

            if (index == self.rowHover)
            {
                inlineColour = ControlCodes::windowColour2;
                drawingCtx.fillRect(rt, 0, y, self.width, y + 9, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
            }

            std::string name(object.second._name);
            name.insert(0, 1, inlineColour);

            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
            AdvancedColour stringColour = Colour::black;
            if (isInUseCompetitor(object.first))
            {
                drawingCtx.setCurrentFontSpriteBase(Font::m1);
                stringColour = self.getColour(WindowColour::secondary).opaque().inset();
            }
            drawingCtx.drawString(rt, 0, y - 1, stringColour, const_cast<char*>(name.c_str()));

            index++;
        }
    }

    static void initEvents()
    {
        events.onClose = onClose;
        events.onMouseUp = onMouseUp;
        events.getScrollSize = getScrollSize;
        events.scrollMouseDown = scrollMouseDown;
        events.scrollMouseOver = scrollMouseOver;
        events.tooltip = tooltip;
        events.prepareDraw = prepareDraw;
        events.draw = draw;
        events.drawScroll = drawScroll;
    }
}
