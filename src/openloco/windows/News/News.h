#pragma once
#include "../../Company.h"
#include "../../Graphics/Gfx.h"
#include "../../Interop/Interop.hpp"
#include "../../Ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::NewsWindow
{
#pragma pack(push, 1)
    struct messageItemType
    {
        uint8_t type;      // 0x00
        uint8_t pad_01[3]; // 0x01
    };
#pragma pack(pop)

    static loco_global<messageItemType[31], 0x004F8B08> _byte_4F8B08;
    static loco_global<messageItemType[31], 0x004F8B09> _byte_4F8B09;
    static loco_global<uint16_t[31], 0x004F8BE4> _word_4F8BE4;
    static loco_global<uint8_t[31], 0x004F8C22> _messageTypes;
    static loco_global<uint8_t[31], 0x004F8C41> _messageSounds;
    static loco_global<uint8_t, 0x00508F1A> _gameSpeed;
    static loco_global<uint8_t[3], 0x005215B5> _unk_5215B5;
    static loco_global<uint32_t, 0x00523338> _cursorX2;
    static loco_global<uint32_t, 0x0052333C> _cursorY2;
    static loco_global<uint32_t, 0x00525CD0> _dword_525CD0;
    static loco_global<uint32_t, 0x00525CD4> _dword_525CD4;
    static loco_global<uint32_t, 0x00525CD8> _dword_525CD8;
    static loco_global<uint32_t, 0x00525CDC> _dword_525CDC;
    static loco_global<uint16_t, 0x00525CE0> _word_525CE0;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint16_t, 0x005271CE> _messageCount;
    static loco_global<uint16_t, 0x005271D0> _activeMessageIndex;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> byte_112CC04;
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    static loco_global<int8_t[8], 0x011364F0> _trackTypesForTab;

    namespace common
    {
        enum widx
        {
            frame,
            close_button,
            viewport1,
            viewport2,
            viewport1Button,
            viewport2Button,
        };

        constexpr uint64_t enabledWidgets = (1 << close_button) | (1 << viewport1Button) | (1 << viewport2Button);

#define commonWidgets(frameWidth, frameHeight, frameType)                                                                                \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, frameType, 0),                                                                     \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        makeWidget({ 2, frameHeight - 73 }, { 168, 64 }, widget_type::viewport, 0, 0xFFFFFFFE),                                          \
        makeWidget({ 180, frameHeight - 73 }, { 168, 64 }, widget_type::viewport, 0, 0xFFFFFFFE),                                        \
        makeWidget({ 2, frameHeight - 75 }, { 180, 75 }, widget_type::wt_9, 0),                                                          \
        makeWidget({ 2, frameHeight - 75 }, { 180, 75 }, widget_type::wt_9, 0)

        void initEvents();
    }

    namespace news1
    {
        static const gfx::ui_size_t windowSize = { 360, 117 };

        extern widget_t widgets[7];

        extern window_event_list events;

        enum newsItemSubTypes
        {
            industry,
            station,
            town,
            vehicle,
            company,
            vehicleTab = 7,
        };

        void initEvents();
        void initViewport(window* self);
    }

    namespace news2
    {
        static const gfx::ui_size_t windowSize = { 360, 159 };

        extern widget_t widgets[7];
    }

    namespace ticker
    {
        static const gfx::ui_size_t windowSize = { 111, 26 };

        enum widx
        {
            frame,
        };
        constexpr uint64_t enabledWidgets = (1 << widx::frame);

        extern widget_t widgets[2];

        extern window_event_list events;

        void initEvents();
    }
}
