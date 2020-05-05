#include "../audio/audio.h"
#include "../companymgr.h"
#include "../core/Optional.hpp"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

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
        make_widget({ 0, 0 }, windowSize, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 398, 13 }, widget_type::caption_24, 0, string_ids::company_face_selection_title),
        make_widget({ 385, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 400, 257 }, widget_type::panel, 1),
        make_widget({ 4, 19 }, { 188, 248 }, widget_type::scrollview, 1, vertical, string_ids::tooltip_company_face_selection),
        make_widget({ 265, 23 }, { 66, 66 }, widget_type::wt_5, 1),
        widget_end(),
    };

    static void initEvents();

    static std::vector<uint32_t> _inUseCompetitors;

    //[[maybe_unused]] static void sub_435381() // VS isn't listening to [[maybe_unused]]
    //{
    //    static loco_global<uint8_t*, 0x009C68CC> _faceSelectionMalloc;
    //    // Zero'd memory
    //    _faceSelectionMalloc = new uint8_t[objectmgr::getNumInstalledObjects()]{};

    //    if (_faceSelectionMalloc == nullptr)
    //    {
    //        exit_with_error(string_ids::null, 0xFF000002);
    //    }
    //}

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

    // 0x004720EB Returns std::nullopt if not loaded
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
            self->init_scroll_widgets();
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

    static void scrollMouseDown(window* const self, const int16_t x, const int16_t y, const uint8_t scroll_index)
    {
        const auto objIndex = getObjectFromSelection(y);

        if (!objIndex.object._header)
        {
            return;
        }
        self->invalidate();
        audio::play_sound(audio::sound_id::click_down, _cursorX);
        gGameCommandErrorTitle = string_ids::cant_select_face;
        const auto result = game_commands::do_65(*objIndex.object._header, self->owner);
        if (result)
        {
            WindowManager::close(self);
        }
    }

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
            const auto colour = colour::get_shade(self->colours[1], 0);
            const auto l = self->x + 1 + self->widgets[widx::face_frame].left;
            const auto t = self->y + 1 + self->widgets[widx::face_frame].top;
            const auto r = self->x - 1 + self->widgets[widx::face_frame].right;
            const auto b = self->y - 1 + self->widgets[widx::face_frame].bottom;
            gfx::fill_rect(dpi, l, t, r, b, colour);

            const competitor_object* competitor = _loadedObject;
            uint32_t img = competitor->images[0] + 1 + (1 << 29);
            gfx::draw_image(dpi, l, t, img);
        }

        {
            const auto x = self->x + self->widgets[widx::face_frame].mid_x();
            const auto y = self->y + self->widgets[widx::face_frame].bottom + 3;
            const auto width = self->width - self->widgets[widx::scrollview].right - 6;
            auto str = const_cast<char*>(stringmgr::get_string(string_ids::buffer_2039));
            *str++ = control_codes::window_colour_2;
            strcpy(str, self->object);
            gfx::draw_string_centred_clipped(*dpi, x, y, width, colour::black, string_ids::buffer_2039);
        }

        // There was code for displaying competitotor stats if window opened with none
        // playing company. But that ability is disabled from the company window.
    }

    static void drawScroll(window* const self, gfx::drawpixelinfo_t* const dpi, const uint32_t scrollIndex)
    {
        gfx::clear_single(*dpi, colour::get_shade(self->colours[1], 4));

        auto index = 0;
        for (const auto& object : objectmgr::getAvailableObjects(object_type::competitor))
        {
            const auto y = index * rowHeight;
            uint8_t inlineColour = control_codes::colour_black;

            if (index == self->row_hover)
            {
                inlineColour = control_codes::window_colour_2;
                gfx::fill_rect(dpi, 0, y, self->width, y + 9, 0x2000000 | 48);
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
            gfx::draw_string(dpi, 0, y - 1, stringColour, const_cast<char*>(name.c_str()));

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
