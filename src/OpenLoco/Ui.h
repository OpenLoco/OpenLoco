#pragma once

#include "Graphics/Gfx.h"
#include <string>
#include <vector>

namespace OpenLoco::Config
{
    enum class screen_mode;
    struct display_config;
    struct resolution_t;
}

namespace OpenLoco::Paint
{
    struct PaintStruct;
}

namespace OpenLoco::Ui
{
    struct viewport;

#pragma pack(push, 1)
    struct screen_info_t
    {
        Gfx::drawpixelinfo_t dpi;
        int16_t width;
        int16_t height;
        int16_t width_2;
        int16_t height_2;
        int16_t width_3;
        int16_t height_3;
        int16_t dirty_block_width;
        int16_t dirty_block_height;
        int32_t dirty_block_columns;
        int32_t dirty_block_rows;
        int8_t dirty_block_column_shift;
        int8_t dirty_block_row_shift;
        int8_t dirty_blocks_initialised;
    };
#pragma pack(pop)

    enum class cursor_id
    {
        pointer,
        blank,
        up_arrow,
        up_down_arrow,
        hand_pointer,
        busy,
        diagonal_arrows,
        unk_25 = 25,
        unk_26 = 26,
        arrows_inward = 37
    };

    struct Resolution
    {
        int32_t width;
        int32_t height;
    };

    namespace ScaleFactor
    {
        const float min = 1.0f;
        const float max = 4.0f;
        const float step = 1.0f;
    };

#ifdef _WIN32
    void* hwnd();
#endif
    int32_t width();
    int32_t height();
    bool dirtyBlocksInitialised();

    void createWindow(const Config::display_config& cfg);
    void initialise();
    void initialiseCursors();
    void initialiseInput();
    void disposeInput();
    void disposeCursors();
    void setCursor(cursor_id id);
    void getCursorPos(int32_t& x, int32_t& y);
    void setCursorPos(int32_t x, int32_t y);
    void hideCursor();
    void showCursor();
    void update();
    void triggerResize();
    void render();
    bool processMessages();
    void showMessageBox(const std::string& title, const std::string& message);
    Config::resolution_t getResolution();
    Config::resolution_t getDesktopResolution();
    bool setDisplayMode(Config::screen_mode mode, Config::resolution_t newResolution);
    bool setDisplayMode(Config::screen_mode mode);
    void updateFullscreenResolutions();
    std::vector<Resolution> getFullscreenResolutions();
    Resolution getClosestResolution(int32_t inWidth, int32_t inHeight);
    void handleInput();
    void minimalHandleInput();
    void setWindowScaling(float newScaleFactor);
    void adjustWindowScale(float adjust_by);

    namespace ViewportInteraction
    {
        enum class InteractionItem : uint8_t
        {
            t_0 = 0,
            t_1 = 1,
            t_2 = 2,
            thing = 3,
            track = 4,
            trackExtra = 5,
            signal = 6,
            trackStation = 7,
            roadStation = 8,
            airport = 9,
            dock = 10,
            t_11 = 11,
            tree = 12,
            wall = 13,
            town = 14,
            station = 15,
            road = 16,
            roadExtra = 17,
            t_18 = 18, // bridge?
            building = 19,
            industry = 20,
            headquarterBuilding = 21,
        };

        struct InteractionArg
        {
            coord_t x;
            coord_t y;
            union
            {
                uint32_t value;
                void* object;
            };
            InteractionItem type;
            uint8_t unkBh;
            InteractionArg() = default;
            InteractionArg(const coord_t _x, const coord_t _y, uint32_t _value, InteractionItem _type, uint8_t _unkBh)
                : x(_x)
                , y(_y)
                , value(_value)
                , type(_type)
                , unkBh(_unkBh)
            {
            }
            InteractionArg(const Paint::PaintStruct& ps);
        };

        InteractionArg getItemLeft(int16_t tempX, int16_t tempY);
        InteractionArg rightOver(int16_t x, int16_t y);

        std::pair<ViewportInteraction::InteractionArg, Ui::viewport*> getMapCoordinatesFromPos(int32_t screenX, int32_t screenY, int32_t flags);
    }
}
