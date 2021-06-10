#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Core/Optional.hpp"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::CompanyFaceSelection
{
    static loco_global<CompanyId_t, 0x9C68F2> _9C68F2; // Use in a game command??
    static loco_global<uint16_t, 0x112C1C1> _numberCompetitorObjects;
    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<CompetitorObject*, 0x0050D15C> _loadedObject; // This could be any type of object
    static loco_global<int32_t, 0x0113E72C> _cursorX;

    static const Gfx::ui_size_t windowSize = { 400, 272 };
    static constexpr uint32_t rowHeight = 10;
    static window_event_list events;

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
    static widget_t widgets[] = {
        makeWidget({ 0, 0 }, windowSize, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 398, 13 }, widget_type::caption_24, 0, StringIds::company_face_selection_title),
        makeWidget({ 385, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 400, 257 }, widget_type::panel, 1),
        makeWidget({ 4, 19 }, { 188, 248 }, widget_type::scrollview, 1, vertical, StringIds::tooltip_company_face_selection),
        makeWidget({ 265, 23 }, { 66, 66 }, widget_type::wt_5, 1),
        widgetEnd(),
    };

    static void initEvents();

    static std::vector<uint32_t> _inUseCompetitors;

    // 0x004353F4
    static void findAllInUseCompetitors(const CompanyId_t id)
    {
        std::vector<uint8_t> takenCompetitorIds;
        for (const auto& c : CompanyManager::companies())
        {
            if (!c.empty() && c.id() != id)
            {
                takenCompetitorIds.push_back(c.competitor_id);
            }
        }

        _inUseCompetitors.clear();
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
        {
            auto competitorId = ObjectManager::findIndex(object.second);
            if (competitorId)
            {
                auto res = std::find(takenCompetitorIds.begin(), takenCompetitorIds.end(), *competitorId);
                if (res != takenCompetitorIds.end())
                {
                    _inUseCompetitors.push_back(object.first);
                }
            }
        }
    }

    // 0x00434F52
    void open(const CompanyId_t id)
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
            self = WindowManager::createWindow(WindowType::companyFaceSelection, windowSize, 0, &events);
            self->widgets = widgets;
            self->enabled_widgets = (1 << widx::close_button);
            self->initScrollWidgets();
            _9C68F2 = id;
            self->owner = id;
            const auto* skin = ObjectManager::get<InterfaceSkinObject>();
            self->colours[1] = skin->colour_0A;
            findAllInUseCompetitors(id);
            self->row_count = _numberCompetitorObjects;
            self->row_hover = -1;
            self->object = nullptr;
        }
    }

    // 0x004352A4
    static void onClose(window* const self)
    {
        ObjectManager::freeScenarioText();
    }

    // 0x435299
    static void onMouseUp(window* const self, const widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(self);
                break;
        }
    }

    // 0x4352BB
    static void getScrollSize(window* const self, const uint32_t scrollIndex, uint16_t* const scrollWidth, uint16_t* const scrollHeight)
    {
        *scrollHeight = _numberCompetitorObjects * rowHeight;
    }

    static bool isInUseCompetitor(const uint32_t objIndex)
    {
        return std::find(_inUseCompetitors.begin(), _inUseCompetitors.end(), objIndex) != _inUseCompetitors.end();
    }

    static ObjectManager::ObjIndexPair getObjectFromSelection(const int16_t& y)
    {
        const int16_t rowIndex = y / rowHeight;
        const auto objects = ObjectManager::getAvailableObjects(ObjectType::competitor);
        if (rowIndex < 0 || static_cast<uint16_t>(rowIndex) >= objects.size())
        {
            return { -1, ObjectManager::ObjectIndexEntry{} };
        }

        if (isInUseCompetitor(objects[rowIndex].first))
        {
            return { -1, ObjectManager::ObjectIndexEntry{} };
        }
        return { rowIndex, objects[rowIndex].second };
    }

    // 0x00435314
    static void scrollMouseDown(window* const self, const int16_t x, const int16_t y, const uint8_t scroll_index)
    {
        const auto objIndex = getObjectFromSelection(y);

        if (!objIndex.object._header)
        {
            return;
        }
        self->invalidate();
        Audio::playSound(Audio::SoundId::clickDown, _cursorX);
        GameCommands::setErrorTitle(StringIds::cant_select_face);
        const auto result = GameCommands::do_65(*objIndex.object._header, self->owner);
        if (result)
        {
            WindowManager::close(self);
        }
    }

    // 0x004352C7
    static void scrollMouseOver(window* const self, const int16_t x, const int16_t y, const uint8_t scroll_index)
    {
        auto [rowIndex, object] = getObjectFromSelection(y);
        if (self->row_hover == rowIndex)
        {
            return;
        }
        self->row_hover = rowIndex;
        self->object = reinterpret_cast<std::byte*>(object._header);
        ObjectManager::freeScenarioText();
        if (object._header)
        {
            ObjectManager::getScenarioText(*object._header);
        }
        self->invalidate();
    }

    // 0x4352B1
    static std::optional<FormatArguments> tooltip(window* const self, const widget_index)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x434FE8
    static void prepareDraw(window* const self)
    {
        const auto company = CompanyManager::get(self->owner);

        FormatArguments args{};
        args.push(company->name);
    }

    // 0x435003
    static void draw(window* const self, Gfx::Context* const context)
    {
        self->draw(context);
        if (self->row_hover == -1)
        {
            return;
        }

        {
            const auto colour = Colour::getShade(self->colours[1], 0);
            const auto l = self->x + 1 + self->widgets[widx::face_frame].left;
            const auto t = self->y + 1 + self->widgets[widx::face_frame].top;
            const auto r = self->x - 1 + self->widgets[widx::face_frame].right;
            const auto b = self->y - 1 + self->widgets[widx::face_frame].bottom;
            Gfx::fillRect(context, l, t, r, b, colour);

            const CompetitorObject* competitor = _loadedObject;
            uint32_t img = competitor->images[0] + 1 + (1 << 29);
            Gfx::drawImage(context, l, t, img);
        }

        {
            const auto x = self->x + self->widgets[widx::face_frame].mid_x();
            const auto y = self->y + self->widgets[widx::face_frame].bottom + 3;
            const auto width = self->width - self->widgets[widx::scrollview].right - 6;
            auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            *str++ = ControlCodes::window_colour_2;
            auto objectPtr = self->object;
            strcpy(str, ObjectManager::ObjectIndexEntry::read(&objectPtr)._name);
            Gfx::drawStringCentredClipped(*context, x, y, width, Colour::black, StringIds::buffer_2039);
        }

        // There was code for displaying competitor stats if window opened with none
        // playing company. But that ability is disabled from the company window.
    }

    // 0x00435152
    static void drawScroll(window* const self, Gfx::Context* const context, const uint32_t scrollIndex)
    {
        Gfx::clearSingle(*context, Colour::getShade(self->colours[1], 4));

        auto index = 0;
        for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::competitor))
        {
            const auto y = index * rowHeight;
            uint8_t inlineColour = ControlCodes::colour_black;

            if (index == self->row_hover)
            {
                inlineColour = ControlCodes::window_colour_2;
                Gfx::fillRect(context, 0, y, self->width, y + 9, 0x2000000 | 48);
            }

            std::string name(object.second._name);
            name.insert(0, 1, inlineColour);

            _currentFontSpriteBase = Font::medium_bold;
            auto stringColour = Colour::black;
            if (isInUseCompetitor(object.first))
            {
                _currentFontSpriteBase = Font::m1;
                stringColour = Colour::opaque(self->colours[1]) | (1 << 6);
            }
            Gfx::drawString(context, 0, y - 1, stringColour, const_cast<char*>(name.c_str()));

            index++;
        }
    }

    static void initEvents()
    {
        events.on_close = onClose;
        events.on_mouse_up = onMouseUp;
        events.get_scroll_size = getScrollSize;
        events.scroll_mouse_down = scrollMouseDown;
        events.scroll_mouse_over = scrollMouseOver;
        events.tooltip = tooltip;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
        events.draw_scroll = drawScroll;
    }
}
