#include <algorithm>
#include <cstring>
#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif
#include "../audio/audio.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../utility/string.hpp"

using namespace openloco::interop;

#ifdef _OPENLOCO_USE_BOOST_FS_
namespace fs = boost::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif

namespace openloco::ui::prompt_browse
{
    static fs::path get_directory(const fs::path& path);
    static std::string get_basename(const fs::path& path);

    enum class browse_file_type
    {
        saved_game,
        landscape,
        unk_2,
    };

#pragma pack(push, 1)
    struct file_entry
    {
    private:
        static constexpr uint8_t flag_directory = 1 << 4;

        uint8_t flags;
        uint8_t unk_01[0x2C - 0x01];
        char filename[0x140 - 0x2C];

    public:
        constexpr bool is_directory()
        {
            return flags & flag_directory;
        }

        std::string_view get_name()
        {
            if (!is_directory())
            {
                auto end = std::strchr(filename, '.');
                if (end != nullptr)
                {
                    return std::string_view(filename, end - filename);
                }
            }
            return filename;
        }
    };
#pragma pack(pop)

    static window_event_list _events;
    static loco_global<uint8_t, 0x009D9D63> _type;
    static loco_global<uint8_t, 0x009DA284> _fileType;
    static loco_global<char[256], 0x009D9D64> _title;
    static loco_global<char[32], 0x009D9E64> _filter;
    static loco_global<char[512], 0x009D9E84> _directory;
    static loco_global<char[512], 0x011369A0> _text_input_buffer;
    static loco_global<int16_t, 0x009D1084> _numFiles;
    static loco_global<file_entry*, 0x0050AEA4> _files;

    static void on_close(window* window);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_update(ui::window* window);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void tooltip(ui::window* window, widget_index widgetIndex);
    static void prepare_draw(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void up_one_level();
    static void sub_446574();

    static void sub_446A93()
    {
        call(0x00446A93);
    }

    static void sub_4CEB67(int16_t dx)
    {
        registers regs;
        regs.dx = dx;
        call(0x004CEB67, regs);
    }

    // 0x00445AB9
    // ecx: path
    // edx: filter
    // ebx: title
    // eax: {return}
    bool open(
        browse_type type,
        char* szPath,
        const char* filter,
        const char* title)
    {
        _events.on_close = on_close;
        _events.on_mouse_up = on_mouse_up;
        _events.on_resize = (uint32_t)0x004467F6;
        _events.on_update = on_update;
        _events.get_scroll_size = get_scroll_size;
        _events.scroll_mouse_down = (uint32_t)0x004464F7;
        _events.scroll_mouse_over = (uint32_t)0x004464B1;
        _events.tooltip = tooltip;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;

        auto path = fs::path(szPath);
        auto directory = get_directory(path);
        auto baseName = get_basename(path);

        textinput::cancel();

        _type = (uint8_t)type;
        _fileType = (uint8_t)browse_file_type::saved_game;
        if (utility::iequals(filter, "*.sc5"))
        {
            _fileType = (uint8_t)browse_file_type::landscape;
        }
        utility::strcpy_safe(_title, title);
        utility::strcpy_safe(_filter, filter);

#ifdef _OPENLOCO_USE_BOOST_FS_
        utility::strcpy_safe(_directory, directory.make_preferred().string().c_str());
#else
        utility::strcpy_safe(_directory, directory.make_preferred().u8string().c_str());
#endif
        utility::strcpy_safe(_text_input_buffer, baseName.c_str());

        sub_446A93();
        auto window = WindowManager::createWindowCentred(WindowType::fileBrowserPrompt, 500, 380, ui::window_flags::stick_to_front | ui::window_flags::resizable | ui::window_flags::flag_12, &_events);
        if (window != nullptr)
        {
            window->widgets = (widget_t*)0x0050AD58;
            window->enabled_widgets = (1 << 2) | (1 << 4) | (1 << 6);
            window->init_scroll_widgets();
            addr<0x01136FA2, int16_t>() = -1;
            addr<0x011370A9, uint8_t>() = 0;
            addr<0x01136FA4, int16_t>() = 0;
            window->var_83E = 11;
            window->var_85A = 0xFFFF;
            addr<0x009DA285, uint8_t>() = 0;
            sub_4CEB67(addr<0x0050ADAC, int16_t>() - addr<0x0050ADAA, int16_t>());
            window->colours[0] = colour::black;
            window->colours[1] = colour::saturated_green;
            WindowManager::setCurrentModalType(WindowType::fileBrowserPrompt);
            prompt_tick_loop(
                []() {
                    input::handle_keyboard();
                    audio::update_sounds();
                    WindowManager::dispatchUpdateAll();
                    call(0x004BEC5B);
                    WindowManager::update();
                    call(0x004C98CF);
                    call(0x004CF63B);
                    return WindowManager::find(WindowType::fileBrowserPrompt) != nullptr;
                });
            WindowManager::setCurrentModalType(WindowType::undefined);
            // TODO: use utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(szPath, _directory);
            if (szPath[0] != '\0')
            {
                return true;
            }
        }
        return false;
    }

    // 0x0044647C
    static void on_close(window*)
    {
        call(0x00446CF4);
        call(0x00447174);
    }

    // 0x00446465
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case 2:
                _directory[0] = '\0';
                WindowManager::close(window);
                break;
            case 6:
                sub_446574();
                break;
            case 4:
                up_one_level();
                window->invalidate();
                break;
        }
    }

    // 0x004467E1
    static void on_update(ui::window* window)
    {
        addr<0x011370A9, uint8_t>()++;
        if ((addr<0x011370A9, uint8_t>() & 0x0F) == 0)
        {
            window->invalidate();
        }
    }

    // 0x004464A1
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->var_83E * _numFiles;
    }

    // 0x004467D7
    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        addr<0x0112C826, string_id>() = string_ids::tooltip_scroll_list;
    }

    // 0x00445C8F
    static void prepare_draw(ui::window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x00445C8F, regs);
    }

    // 0x00445E38
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        registers regs;
        regs.esi = (int32_t)window;
        regs.edi = (int32_t)dpi;
        call(0x00445E38, regs);
    }

    // 0x00446314
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        loco_global<uint8_t[256], 0x001136BA4> byte_1136BA4;
        loco_global<char[16], 0x0112C826> _commonFormatArgs;

        static std::string _nameBuffer;

        // Background
        auto paletteId = byte_1136BA4[window->colours[1] * 8];
        gfx::clear_single(*dpi, paletteId);

        // Directories / files
        auto y = 0;
        auto lineHeight = window->var_83E;
        for (auto i = 0; i < _numFiles; i++)
        {
            if (y + lineHeight >= dpi->y && y <= dpi->y + dpi->height)
            {
                auto file = _files[i];

                // Draw the row highlight
                auto stringId = string_ids::white_stringid2;
                if (i == window->var_85A)
                {
                    gfx::draw_rect(dpi, 0, y, window->width, lineHeight, 0x2000000 | 48);
                    stringId = string_ids::wcolour2_stringid2;
                }

                // Draw the folder icon
                auto x = 1;
                if (file.is_directory())
                {
                    gfx::draw_image(dpi, x, y, image_ids::icon_folder);
                    x += 14;
                }

                // Copy name to our work buffer
                _nameBuffer = file.get_name();

                // Draw the name
                auto name = _nameBuffer.c_str();
                auto stringId2 = string_ids::stringptr;
                std::memcpy(_commonFormatArgs, &stringId2, sizeof(stringId2));
                std::memcpy(_commonFormatArgs + 2, &name, sizeof(name));
                gfx::draw_string_494B3F(*dpi, x, y, 0, stringId, _commonFormatArgs);
            }
            y += lineHeight;
        }
    }

    static fs::path get_directory(const fs::path& path)
    {
        if (path.has_extension())
        {
            std::basic_string<fs::path::value_type> sep(1, fs::path::preferred_separator);
            return path.parent_path().concat(sep);
        }
        else
        {
            auto str = path.string();
            if (str.size() > 0)
            {
                auto lastCharacter = str[str.size() - 1];
                if (lastCharacter == fs::path::preferred_separator)
                {
                    return path;
                }
            }
            return fs::path();
        }
    }

    static std::string get_basename(const fs::path& path)
    {
#ifdef _OPENLOCO_USE_BOOST_FS_
        auto baseName = path.stem().string();
#else
        auto baseName = path.stem().u8string();
#endif
        if (baseName == ".")
        {
            baseName = "";
        }
        return baseName;
    }

    // 0x00446A93
    static void refresh_directory_list()
    {
        call(0x00446A93);
    }

    // 0x00446E2F
    static void up_one_level()
    {
        char* ptr = _directory;
        while (*ptr != '\0')
            ptr++;

        ptr--;
        if (*ptr == fs::path::preferred_separator)
            ptr--;

        while (ptr != _directory && *ptr != fs::path::preferred_separator)
            ptr--;

        if (*ptr == fs::path::preferred_separator)
            ptr++;

        *ptr = '\0';

        refresh_directory_list();
    }

    // 0x00446E62
    static void append_directory(const char* to_append)
    {
        char* dst = _directory;
        while (*dst != '\0')
            dst++;

        const char* src = to_append;
        while (*src != '\0')
        {
            *dst++ = *src++;
        }

        *dst++ = fs::path::preferred_separator;
        *dst = '\0';

        refresh_directory_list();
    }

    void register_hooks()
    {
        register_hook(
            0x00445AB9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto result = open(
                    (browse_type)regs.al,
                    (char*)regs.ecx,
                    (const char*)regs.edx,
                    (const char*)regs.ebx);
                regs.eax = result ? 1 : 0;
                return 0;
            });

        register_hook(
            0x00446E2F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                up_one_level();
                return 0;
            });

        register_hook(
            0x00446E62,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                append_directory((char*)regs.ebp);
                return 0;
            });
    }

    static int sub_446F1D()
    {
        return call(0x00446F1D) & X86_FLAG_CARRY;
    }

    // 0x00446574
    static void sub_446574()
    {
        if (_type != (uint8_t)browse_file_type::unk_2)
        {
            call(0x00446689);
        }
        else
        {
            if (sub_446F1D())
            {
                windows::show_error(string_ids::error_invalid_filename, 0xFFFF);
            }
            else
            {
                call(0x00446598);
            }
        }
    }
}
