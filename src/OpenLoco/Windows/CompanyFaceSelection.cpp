#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Core/Optional.hpp"
#include "../GameCommands.h"
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

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyFaceSelection
{
    static loco_global<company_id_t, 0x9C68F2> _9C68F2; // Use in a game command??
    static loco_global<uint16_t, 0x112C1C1> _numberCompetitorObjects;
    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<competitor_object*, 0x0050D15C> _loadedObject; // This could be any type of object
    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;

    static const gfx::ui_size_t windowSize = { 400, 272 };
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
        makeWidget({ 1, 1 }, { 398, 13 }, widget_type::caption_24, 0, string_ids::company_face_selection_title),
        makeWidget({ 385, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        makeWidget({ 0, 15 }, { 400, 257 }, widget_type::panel, 1),
        makeWidget({ 4, 19 }, { 188, 248 }, widget_type::scrollview, 1, vertical, string_ids::tooltip_company_face_selection),
        makeWidget({ 265, 23 }, { 66, 66 }, widget_type::wt_5, 1),
        widgetEnd(),
    };

    static void initEvents();

    static std::vector<uint32_t> _inUseCompetitors;

    // Object free?
    static void sub_471B95()
    {
        call(0x00471B95);
    }

    // Object load?
    static void sub_47176D(objectmgr::header& object)
    {
        registers regs;
        regs.ebp = reinterpret_cast<int32_t>(&object);
        call(0x0047176D, regs);
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    static std::optional<uint32_t> getLoadedObjectIndex(const objectmgr::object_index_entry& object)
    {
        registers regs;
        regs.ebp = reinterpret_cast<uint32_t>(&object._header->type);
        const bool success = !(call(0x004720EB, regs) & (X86_FLAG_CARRY << 8));
        // Object type is also returned on ecx
        if (success)
        {
            return { regs.ebx };
        }
        return std::nullopt;
    }

    // 0x004353F4
    static void findAllInUseCompetitors(const company_id_t id)
    {
        std::vector<uint8_t> takenCompetitorIds;
        for (const auto& c : companymgr::companies())
        {
            if (!c.empty() && c.id() != id)
            {
                takenCompetitorIds.push_back(c.competitor_id);
            }
        }

        _inUseCompetitors.clear();
        for (const auto& object : objectmgr::getAvailableObjects(object_type::competitor))
        {
            auto competitorId = getLoadedObjectIndex(object.second);
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
    void open(const company_id_t id)
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
            const auto* skin = objectmgr::get<interface_skin_object>();
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
        sub_471B95();
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

    struct ObjIndexPair
    {
        int16_t index;
        objectmgr::object_index_entry object;
    };

    // 0x004354A6 sort of, very different
    static ObjIndexPair getObjectFromSelection(const int16_t& y)
    {
        const int16_t rowIndex = y / rowHeight;
        const auto objects = objectmgr::getAvailableObjects(object_type::competitor);
        if (rowIndex < 0 || static_cast<uint16_t>(rowIndex) >= objects.size())
        {
            return { -1, objectmgr::object_index_entry{} };
        }

        if (isInUseCompetitor(objects[rowIndex].first))
        {
            return { -1, objectmgr::object_index_entry{} };
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
        audio::playSound(audio::sound_id::click_down, _cursorX);
        gGameCommandErrorTitle = string_ids::cant_select_face;
        const auto result = game_commands::do_65(*objIndex.object._header, self->owner);
        if (result)
        {
            WindowManager::close(self);
        }
    }

    // 0x004352C7
    static void scrollMouseOver(window* const self, const int16_t x, const int16_t y, const uint8_t scroll_index)
    {
        const auto [rowIndex, object] = getObjectFromSelection(y);
        if (self->row_hover == rowIndex)
        {
            return;
        }
        self->row_hover = rowIndex;
        self->object = object._name;
        sub_471B95();
        if (object._header)
        {
            sub_47176D(*object._header);
        }
        self->invalidate();
    }

    // 0x4352B1
    static void tooltip(FormatArguments& args, window* const self, const widget_index)
    {
        args.push(string_ids::tooltip_scroll_list);
    }

    // 0x434FE8
    static void prepareDraw(window* const self)
    {
        const auto company = companymgr::get(self->owner);

        FormatArguments args{};
        args.push(company->name);
    }

    // 0x435003
    static void draw(window* const self, gfx::drawpixelinfo_t* const dpi)
    {
        self->draw(dpi);
        if (self->row_hover == -1)
        {
            return;
        }

        {
            const auto colour = colour::getShade(self->colours[1], 0);
            const auto l = self->x + 1 + self->widgets[widx::face_frame].left;
            const auto t = self->y + 1 + self->widgets[widx::face_frame].top;
            const auto r = self->x - 1 + self->widgets[widx::face_frame].right;
            const auto b = self->y - 1 + self->widgets[widx::face_frame].bottom;
            gfx::fillRect(dpi, l, t, r, b, colour);

            const competitor_object* competitor = _loadedObject;
            uint32_t img = competitor->images[0] + 1 + (1 << 29);
            gfx::drawImage(dpi, l, t, img);
        }

        {
            const auto x = self->x + self->widgets[widx::face_frame].mid_x();
            const auto y = self->y + self->widgets[widx::face_frame].bottom + 3;
            const auto width = self->width - self->widgets[widx::scrollview].right - 6;
            auto str = const_cast<char*>(stringmgr::getString(string_ids::buffer_2039));
            *str++ = control_codes::window_colour_2;
            strcpy(str, self->object);
            gfx::drawStringCentredClipped(*dpi, x, y, width, colour::black, string_ids::buffer_2039);
        }

        // There was code for displaying competitor stats if window opened with none
        // playing company. But that ability is disabled from the company window.
    }

    // 0x00435152
    static void drawScroll(window* const self, gfx::drawpixelinfo_t* const dpi, const uint32_t scrollIndex)
    {
        gfx::clearSingle(*dpi, colour::getShade(self->colours[1], 4));

        auto index = 0;
        for (const auto& object : objectmgr::getAvailableObjects(object_type::competitor))
        {
            const auto y = index * rowHeight;
            uint8_t inlineColour = control_codes::colour_black;

            if (index == self->row_hover)
            {
                inlineColour = control_codes::window_colour_2;
                gfx::fillRect(dpi, 0, y, self->width, y + 9, 0x2000000 | 48);
            }

            std::string name(object.second._name);
            name.insert(0, 1, inlineColour);

            _currentFontSpriteBase = font::medium_bold;
            auto stringColour = colour::black;
            if (isInUseCompetitor(object.first))
            {
                _currentFontSpriteBase = font::m1;
                stringColour = colour::opaque(self->colours[1]) | (1 << 6);
            }
            gfx::drawString(dpi, 0, y - 1, stringColour, const_cast<char*>(name.c_str()));

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
