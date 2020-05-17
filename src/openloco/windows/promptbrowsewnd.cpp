#include "../audio/audio.h"
#include "../core/FileSystem.hpp"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../platform/platform.h"
#include "../s5/s5.h"
#include "../scenario.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../utility/string.hpp"
#include <algorithm>
#include <cstring>

using namespace openloco::interop;

namespace openloco::ui::prompt_browse
{
    static fs::path get_directory(const fs::path& path);
    static std::string get_basename(const fs::path& path);

    enum browse_file_type : uint8_t
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

        uint8_t flags{};
        uint8_t unk_01[0x2C - 0x01]{};
        char filename[0x140 - 0x2C]{};

    public:
        file_entry()
        {
        }

        file_entry(const std::string_view& p, bool isDirectory)
        {
            p.copy(filename, sizeof(filename) - 1, 0);
            flags = isDirectory ? flag_directory : 0;
        }

        constexpr bool is_directory() const
        {
            return flags & flag_directory;
        }

        std::string_view get_name() const
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

    enum widx
    {
        frame,
        caption,
        close_button,
        panel,
        parent_button,
        text_filename,
        ok_button,
        scrollview,
    };

    static widget_t widgets[] = {
        make_widget({ 0, 0 }, { 500, 380 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 498, 13 }, widget_type::caption_25, 0, string_ids::buffer_2039),
        make_widget({ 485, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 500, 365 }, widget_type::panel, 1),
        make_widget({ 473, 18 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::icon_parent_folder, string_ids::window_browse_parent_folder_tooltip),
        make_widget({ 88, 348 }, { 408, 14 }, widget_type::wt_17, 1),
        make_widget({ 426, 364 }, { 70, 12 }, widget_type::wt_11, 1, string_ids::label_button_ok),
        make_widget({ 3, 45 }, { 494, 323 }, widget_type::scrollview, 1, vertical),
        widget_end(),
    };

    static window_event_list _events;
    static loco_global<uint8_t, 0x009D9D63> _type;
    static loco_global<uint8_t, 0x009DA284> _fileType;
    static loco_global<char[256], 0x009D9D64> _title;
    static loco_global<char[32], 0x009D9E64> _filter;
    static loco_global<char[512], 0x009D9E84> _directory;
    static loco_global<char[512], 0x011369A0> _text_input_buffer;
    static loco_global<int16_t, 0x009D1084> _numFiles;
    static loco_global<file_entry*, 0x0050AEA4> _files;
    static loco_global<int16_t, 0x01136FA2> _textInputCaret;
    static loco_global<uint8_t, 0x01136FA4> _textInputLeft;
    static loco_global<uint8_t, 0x011370A9> _textInputFlags;

    static std::vector<file_entry> _newFiles;

    static void on_close(window* window);
    static void on_resize(window* window);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_update(ui::window* window);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex);
    static void prepare_draw(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_save_preview(ui::window& window, gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height, const s5::SaveDetails& saveInfo);
    static void draw_landscape_preview(ui::window& window, gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height);
    static void draw_text_input(ui::window* window, gfx::drawpixelinfo_t& dpi, const char* text, int32_t caret, bool showCaret);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void up_one_level();
    static void sub_446574(ui::window* window);
    static void refresh_directory_list();

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
        _events.on_resize = on_resize;
        _events.on_update = on_update;
        _events.get_scroll_size = get_scroll_size;
        _events.scroll_mouse_down = reinterpret_cast<void (*)(window*, int16_t, int16_t, uint8_t)>(0x004464F7);
        _events.scroll_mouse_over = reinterpret_cast<void (*)(window*, int16_t, int16_t, uint8_t)>(0x004464B1);
        _events.tooltip = tooltip;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;

        auto path = fs::path(szPath);
        auto directory = get_directory(path);
        auto baseName = get_basename(path);

        textinput::cancel();

        *_type = type;
        *_fileType = browse_file_type::saved_game;
        if (utility::iequals(filter, "*.sc5"))
        {
            *_fileType = browse_file_type::landscape;
        }
        utility::strcpy_safe(_title, title);
        utility::strcpy_safe(_filter, filter);

        utility::strcpy_safe(_directory, directory.make_preferred().u8string().c_str());
        utility::strcpy_safe(_text_input_buffer, baseName.c_str());

        refresh_directory_list();

        auto window = WindowManager::createWindowCentred(
            WindowType::fileBrowserPrompt,
            { 500, 380 },
            ui::window_flags::stick_to_front | ui::window_flags::resizable | ui::window_flags::flag_12,
            &_events);

        if (window != nullptr)
        {
            window->widgets = widgets;
            window->enabled_widgets = (1 << widx::close_button) | (1 << widx::parent_button) | (1 << widx::ok_button);
            window->init_scroll_widgets();

            _textInputCaret = -1;
            _textInputFlags = 0;
            _textInputLeft = 0;

            window->row_height = 11;
            window->var_85A = -1;

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
                    input::process_keyboard_input();
                    WindowManager::update();
                    ui::minimalHandleInput();
                    gfx::render();
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
        _newFiles = {};
        _numFiles = 0;
        _files = (file_entry*)-1;

        call(0x00447174);
    }

    // 0x004467F6
    static void on_resize(window* window)
    {
        window->cap_size(400, 300, 640, 800);
    }

    // 0x00446465
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                _directory[0] = '\0';
                WindowManager::close(window);
                break;
            case widx::parent_button:
                up_one_level();
                window->invalidate();
                break;
            case widx::ok_button:
                sub_446574(window);
                break;
        }
    }

    // 0x004467E1
    static void on_update(ui::window* window)
    {
        _textInputFlags++;
        if ((_textInputFlags & 0x0F) == 0)
        {
            window->invalidate();
        }
    }

    // 0x004464A1
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->row_height * _numFiles;
    }

    // 0x004467D7
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
    {
        args.push(string_ids::tooltip_scroll_list);
    }

    // 0x00445C8F
    static void prepare_draw(ui::window* self)
    {
        // TODO: replace with a fixed length!
        char* buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
        strcpy(buffer, _title);

        self->widgets[widx::frame].right = self->width - 1;
        self->widgets[widx::frame].bottom = self->height - 1;

        self->widgets[widx::panel].right = self->width - 1;
        self->widgets[widx::panel].bottom = self->height - 1;

        self->widgets[widx::caption].right = self->width - 2;

        self->widgets[widx::close_button].left = self->width - 15;
        self->widgets[widx::close_button].right = self->width - 3;

        if (*_type == browse_type::save)
        {
            self->widgets[widx::ok_button].left = self->width - 86;
            self->widgets[widx::ok_button].right = self->width - 16;
            self->widgets[widx::ok_button].top = self->height - 15;
            self->widgets[widx::ok_button].bottom = self->height - 4;
            self->widgets[widx::ok_button].type = widget_type::wt_11;

            self->widgets[widx::text_filename].right = self->width - 4;
            self->widgets[widx::text_filename].top = self->height - 31;
            self->widgets[widx::text_filename].bottom = self->height - 18;
            self->widgets[widx::text_filename].type = widget_type::wt_17;

            self->widgets[widx::scrollview].bottom = self->height - 34;
        }
        else
        {
            self->widgets[widx::ok_button].type = widget_type::none;
            self->widgets[widx::text_filename].type = widget_type::none;

            self->widgets[widx::scrollview].bottom = self->height - 4;
        }

        self->widgets[widx::scrollview].right = self->width - 259;
        if (*_fileType != browse_file_type::saved_game)
            self->widgets[widx::scrollview].right += 122;

        self->widgets[widx::parent_button].left = self->width - 26;
        self->widgets[widx::parent_button].right = self->width - 3;

        // Resume the original prepare_draw routine beyond the widget repositioning.
        registers regs;
        regs.edi = (int32_t)buffer;
        regs.esi = (int32_t)self;
        call(0x00445D91, regs);
    }

    static void set_common_args_stringptr(const char* buffer)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;
        auto stringptrId = string_ids::stringptr;
        std::memcpy(_commonFormatArgs, &stringptrId, sizeof(stringptrId));
        std::memcpy(_commonFormatArgs + 2, &buffer, sizeof(buffer));
    }

    // 0x00445E38
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;
        static std::string _nameBuffer;

        window->draw(dpi);

        auto folder = (const char*)0x9DA084;
        set_common_args_stringptr(folder);
        gfx::draw_string_494B3F(*dpi, window->x + 3, window->y + window->widgets[widx::parent_button].top + 6, 0, string_ids::window_browse_folder, _commonFormatArgs);

        auto selectedIndex = window->var_85A;
        if (selectedIndex != -1)
        {
            auto& selectedFile = _files[selectedIndex];
            if (!selectedFile.is_directory())
            {
                const auto& widget = window->widgets[widx::scrollview];

                auto width = window->width - widget.right - 8;
                auto x = window->x + widget.right + 3;
                auto y = window->y + 45;

                _nameBuffer = selectedFile.get_name();
                set_common_args_stringptr(_nameBuffer.c_str());
                gfx::draw_string_centred_clipped(
                    *dpi,
                    x + (width / 2),
                    y,
                    width,
                    0,
                    string_ids::wcolour2_stringid,
                    _commonFormatArgs);
                y += 12;

                if (*_fileType == browse_file_type::saved_game)
                {
                    // Preview image
                    auto saveInfo = *((const s5::SaveDetails**)0x50AEA8);
                    if (saveInfo != (void*)-1)
                    {
                        draw_save_preview(*window, *dpi, x, y, width, 201, *saveInfo);
                    }
                }
                else if (*_fileType == browse_file_type::landscape)
                {
                    draw_landscape_preview(*window, *dpi, x, y, width, 129);
                }
            }
        }

        const auto& filenameBox = window->widgets[widx::text_filename];
        if (filenameBox.type != widget_type::none)
        {
            // Draw filename label
            gfx::draw_string_494B3F(*dpi, window->x + 3, window->y + filenameBox.top + 2, 0, string_ids::window_browse_filename, nullptr);

            // Clip to text box
            gfx::drawpixelinfo_t* dpi2;
            if (gfx::clip_drawpixelinfo(
                    &dpi2,
                    dpi,
                    window->x + filenameBox.left + 1,
                    window->y + filenameBox.top + 1,
                    filenameBox.right - filenameBox.left - 1,
                    filenameBox.bottom - filenameBox.top - 1))
            {
                draw_text_input(window, *dpi2, _text_input_buffer, _textInputCaret, (_textInputFlags & 0x10) == 0);
            }
        }
    }

    static void draw_save_preview(ui::window& window, gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height, const s5::SaveDetails& saveInfo)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;

        gfx::fill_rect_inset(&dpi, x, y, x + width, y + height, window.colours[1], 0x30);

        auto imageId = 0;
        auto g1 = gfx::get_g1element(imageId);
        if (g1 != nullptr)
        {
            // Temporarily substitute a g1 for the image data in the saved game
            auto backupg1 = *g1;
            *g1 = {};
            g1->offset = (uint8_t*)saveInfo.image;
            g1->width = 250;
            g1->height = 200;
            gfx::draw_image(&dpi, x + 1, y + 1, imageId);
            *g1 = backupg1;
        }
        y += 207;

        uint16_t maxWidth = window.width - window.widgets[widx::scrollview].right;

        // Company
        set_common_args_stringptr(saveInfo.company);
        y = gfx::draw_string_495224(dpi, x, y, maxWidth, colour::black, string_ids::window_browse_company, _commonFormatArgs);

        // Owner
        set_common_args_stringptr(saveInfo.owner);
        y = gfx::draw_string_495224(dpi, x, y, maxWidth, colour::black, string_ids::owner_label, _commonFormatArgs);

        // Date
        y = gfx::draw_string_495224(dpi, x, y, maxWidth, colour::black, string_ids::window_browse_date, &saveInfo.date);

        // Challenge progress
        auto flags = saveInfo.challenge_flags;
        if (!(flags & company_flags::challenge_beaten_by_opponent))
        {
            auto stringId = string_ids::window_browse_challenge_completed;
            int16_t progress = 0;
            if (!(flags & company_flags::challenge_completed))
            {
                stringId = string_ids::window_browse_challenge_failed;
                if (!(flags & company_flags::challenge_failed))
                {
                    stringId = string_ids::window_browse_challenge_progress;
                    progress = saveInfo.challenge_progress;
                }
            }
            gfx::draw_string_495224(dpi, x, y, maxWidth, colour::black, stringId, &progress);
        }
    }

    static void draw_landscape_preview(ui::window& window, gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        gfx::fill_rect_inset(&dpi, x, y, x + width, y + height, window.colours[1], 0x30);

        if (s5::getPreviewOptions().scenarioFlags & scenario::flags::landscape_generation_done)
        {
            // Height map
            auto imageId = 0;
            auto g1 = gfx::get_g1element(imageId);
            if (g1 != nullptr)
            {
                // Temporarily substitute a g1 for the height map image data in the saved game
                auto backupg1 = *g1;
                *g1 = {};
                g1->offset = (uint8_t*)0x9CCBDE;
                g1->width = 128;
                g1->height = 128;
                gfx::draw_image(&dpi, x + 1, y + 1, imageId);
                *g1 = backupg1;

                gfx::draw_image(&dpi, x, y + 1, image_ids::height_map_compass);
            }
        }
        else
        {
            // Randomly generated landscape
            auto imageId = image_ids::random_map_watermark | (window.colours[1] << 19) | 0x20000000;
            gfx::draw_image(&dpi, x, y, imageId);
            gfx::point_t origin = { (int16_t)(x + 64), (int16_t)(y + 60) };
            gfx::draw_string_centred_wrapped(&dpi, &origin, 128, 0, string_ids::randomly_generated_landscape);
        }
    }

    static void draw_text_input(ui::window* window, gfx::drawpixelinfo_t& dpi, const char* text, int32_t caret, bool showCaret)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;
        loco_global<uint8_t[256], 0x001136C99> byte_1136C99;
        static std::string gbuffer;

        // Draw text box text
        gfx::point_t origin = { 0, 1 };
        set_common_args_stringptr(text);
        gfx::draw_string_494B3F(dpi, &origin, 0, string_ids::black_stringid, _commonFormatArgs);

        if (showCaret)
        {
            if (caret == -1)
            {
                // Draw horizontal caret
                gfx::draw_string_494B3F(dpi, &origin, 0, string_ids::window_browse_input_caret, nullptr);
            }
            else
            {
                // Draw text[0:caret] over the top
                // TODO this should really just be measuring the string
                gbuffer = std::string_view(text, caret);
                set_common_args_stringptr(gbuffer.c_str());
                origin = { 0, 1 };
                gfx::draw_string_494B3F(dpi, &origin, 0, string_ids::black_stringid, _commonFormatArgs);

                // Draw vertical caret
                gfx::draw_rect(&dpi, origin.x, origin.y, 1, 9, byte_1136C99[window->colours[1] * 8]);
            }
        }
    }

    // 0x00446314
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;

        static std::string _nameBuffer;

        // Background
        gfx::clear_single(*dpi, colour::get_shade(window->colours[1], 4));

        // Directories / files
        auto y = 0;
        auto lineHeight = window->row_height;
        for (auto i = 0; i < _numFiles; i++)
        {
            if (y + lineHeight >= dpi->y && y <= dpi->y + dpi->height)
            {
                auto file = _files[i];

                // Draw the row highlight
                auto stringId = string_ids::black_stringid;
                if (i == window->var_85A)
                {
                    gfx::draw_rect(dpi, 0, y, window->width, lineHeight, 0x2000000 | 48);
                    stringId = string_ids::wcolour2_stringid;
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
                set_common_args_stringptr(_nameBuffer.c_str());
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
        auto baseName = path.stem().u8string();
        if (baseName == ".")
        {
            baseName = "";
        }
        return baseName;
    }

    // 0x00446A93
    static void refresh_directory_list()
    {
        // All our filters are probably *.something so just truncate the *
        // and treat as an extension filter
        auto filterExtension = std::string(_filter);
        if (filterExtension[0] == '*')
        {
            filterExtension = filterExtension.substr(1);
        }

        _newFiles.clear();
        if (_directory[0] == '\0')
        {
            auto drives = platform::getDrives();
            for (auto& drive : drives)
            {
                auto name = drive.u8string();
                _newFiles.emplace_back(name, true);
            }
        }
        else
        {
            auto directory = fs::path((char*)_directory);
            if (fs::is_directory(directory))
            {
                for (auto& f : fs::directory_iterator(directory))
                {
                    bool isDirectory = fs::is_directory(f);
                    if (!isDirectory)
                    {
                        auto extension = f.path().extension().u8string();
                        if (!utility::iequals(extension, filterExtension))
                        {
                            continue;
                        }
                    }

                    auto name = f.path().stem().u8string();
                    _newFiles.emplace_back(name, isDirectory);
                }
            }
        }

        std::sort(_newFiles.begin(), _newFiles.end(), [](const file_entry& a, const file_entry& b) -> bool {
            if (!a.is_directory() && b.is_directory())
                return false;
            if (a.is_directory() && !b.is_directory())
                return true;
            return a.get_name() < b.get_name();
        });

        _numFiles = (int16_t)_newFiles.size();
        _files = _newFiles.data();
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
    static void sub_446574(ui::window* window)
    {
        if (*_type != browse_file_type::unk_2)
        {
            call(0x00446689);
        }
        else
        {
            if (sub_446F1D())
            {
                windows::show_error(string_ids::error_invalid_filename);
            }
            else
            {
                registers regs;
                regs.esi = (int32_t)window;
                call(0x00446598, regs);
            }
        }
    }
}
