#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"
#include <optional>

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyFaceSelection
{
    static loco_global<uint8_t, 0x9C68F2> _9C68F2;
    static loco_global<uint16_t, 0x112C1C1> _112C1C1;
    static loco_global<uint8_t*, 0x009C68CC> _faceSelectionMalloc;
    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;

    static const gfx::ui_size_t windowSize = { 400, 272 };
    static window_event_list events;

    enum widx
    {
        frame,
        caption,
        close_button,
        panel,
        scrollview,
        unk_5
    };

    // 0x509680
    static widget_t widgets[] = {
        make_widget({ 0, 0 }, { 400, 272 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 398, 13 }, widget_type::caption_24, 0, string_ids::company_face_selection_title),
        make_widget({ 385, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 400, 257 }, widget_type::panel, 1),
        make_widget({ 4, 19 }, { 188, 248 }, widget_type::scrollview, 1, vertical, string_ids::tooltip_company_face_selection),
        make_widget({ 265, 23 }, { 66, 66 }, widget_type::wt_5, 1),
        widget_end(),
    };

    static void initEvents();

    void sub_435381()
    {
        // Zero'd memory
        _faceSelectionMalloc = new uint8_t[objectmgr::getNumInstalledObjects()]{};

        if (_faceSelectionMalloc == nullptr)
        {
            exit_with_error(string_ids::null, 0xFF000002);
        }
    }

    struct unk
    {
        object_type objectType;
        int32_t index;
    };

    // 0x004720EB Returns std::nullopt if not loaded
    std::optional<unk> getLoadedObjectTypeAndIndex(const objectmgr::object_index_entry& object)
    {
        registers regs;
        regs.ebp = reinterpret_cast<uint32_t>(&object._header->type);
        bool success = !(call(0x004720EB, regs) & (X86_FLAG_CARRY << 8));
        if (success)
        {
            return { { static_cast<object_type>(regs.ecx), regs.ebx } };
        }
        return std::nullopt;
    }

    void sub_4353F4(company_id_t id)
    {
        std::vector<uint8_t> takenCompetitorIds;
        for (const auto& c : companymgr::companies())
        {
            if (!c.empty() && c.id() != id)
            {
                takenCompetitorIds.push_back(c.competitor_id);
            }
        }

        for (auto object : objectmgr::getAvailableObjects(object_type::competitor))
        {
            auto objectRef = getLoadedObjectTypeAndIndex(object.second);
            if (objectRef)
            {
                if (objectRef->objectType == object_type::competitor)
                {
                    auto res = std::find(takenCompetitorIds.begin(), takenCompetitorIds.end(), objectRef->index);
                    if (res != takenCompetitorIds.end())
                    {
                        _faceSelectionMalloc[object.first] = 0x20;
                    }
                }
            }
        }
    }

    void open(company_id_t id)
    {
        auto self = WindowManager::bringToFront(WindowType::companyFaceSelection, 0);

        if (self)
        {
            _9C68F2 = id;
            self->owner = id;
            sub_4353F4(id);
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
            auto skin = objectmgr::get<interface_skin_object>();
            self->colours[1] = skin->colour_0A;
            sub_435381();
            sub_4353F4(id);
            self->row_count = _112C1C1;
            self->row_hover = -1;
            self->var_85A = -1;
            self->var_85C = -1;
        }
    }

    static void onClose(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x4352A4, regs);
    }

    // 0x435299
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(self);
                break;
        }
    }

    // 0x4352BB
    static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = _112C1C1 * 10;
    }

    static void scrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)self;
        regs.cx = x;
        regs.dx = y;
        call(0x435314, regs);
    }

    static void scrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)self;
        regs.cx = x;
        regs.dx = y;
        call(0x004352C7, regs);
    }

    // 0x4352B1
    static void tooltip(FormatArguments& args, window* self, widget_index)
    {
        args.push(string_ids::tooltip_scroll_list);
    }

    // 0x434FE8
    static void prepareDraw(window* self)
    {
        auto company = companymgr::get(_9C68F2);

        FormatArguments args{};
        args.push(company->name);
    }

    // 0x435003
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        if (self->row_hover == -1)
        {
            return;
        }

        registers regs;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;
        call(0x435003, regs);
    }

    static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        gfx::clear_single(*dpi, colour::get_shade(self->colours[1], 4));
        auto y = 0;
        auto nameCpy = std::make_unique<char[]>(256);
        for (auto object : objectmgr::getAvailableObjects(object_type::competitor))
        {
            uint8_t inlineColour = control_codes::colour_black;

            bool isDisabled = _faceSelectionMalloc[object.first] & (0x20);
            if (object.second._header == reinterpret_cast<objectmgr::header*>(*(uint32_t*)&self->var_85A))
            {
                inlineColour = control_codes::window_colour_2;
                gfx::fill_rect(dpi, 0, y, self->width, y + 9, 0x2000000 | 48);
            }

            strcpy(nameCpy.get() + 1, object.second._name);
            nameCpy[0] = inlineColour;
            _currentFontSpriteBase = font::medium_bold;
            auto stringColour = colour::black;
            if (isDisabled)
            {
                _currentFontSpriteBase = font::m1;
                stringColour = colour::opaque(self->colours[1]) | (1 << 6);
            }
            gfx::draw_string(dpi, 0, y - 1, stringColour, nameCpy.get());

            y += 10;
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
