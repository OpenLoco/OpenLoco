#include "../Audio/Audio.h"
#include "../Console.h"
#include "../Core/FileSystem.hpp"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Platform/Platform.h"
#include "../Ptr.h"
#include "../S5/S5.h"
#include "../Scenario.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Utility/String.hpp"
#include <algorithm>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::PromptBrowse
{
    static fs::path getDirectory(const fs::path& path);
    static std::string getBasename(const fs::path& path);

    enum browse_file_type : uint8_t
    {
        saved_game,
        landscape,
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

    static_assert(sizeof(file_entry) == 0x140);

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
        makeWidget({ 0, 0 }, { 500, 380 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 498, 13 }, widget_type::caption_25, 0, StringIds::buffer_2039),
        makeWidget({ 485, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 500, 365 }, widget_type::panel, 1),
        makeWidget({ 473, 18 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::icon_parent_folder, StringIds::window_browse_parent_folder_tooltip),
        makeWidget({ 88, 348 }, { 408, 14 }, widget_type::wt_17, 1),
        makeWidget({ 426, 364 }, { 70, 12 }, widget_type::wt_11, 1, StringIds::label_button_ok),
        makeWidget({ 3, 45 }, { 494, 323 }, widget_type::scrollview, 1, vertical),
        widgetEnd(),
    };

    static window_event_list _events;
    static loco_global<uint8_t, 0x009D9D63> _type;
    static loco_global<browse_file_type, 0x009DA284> _fileType;
    static loco_global<char[256], 0x009D9D64> _title;
    static loco_global<char[32], 0x009D9E64> _filter;
    static loco_global<char[512], 0x009D9E84> _directory;
    static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;
    static loco_global<char[512], 0x011369A0> _text_input_buffer;
    static loco_global<int16_t, 0x009D1084> _numFiles;
    static loco_global<file_entry*, 0x0050AEA4> _files;
    static loco_global<int16_t, 0x01136FA2> _textInputCaret;
    static loco_global<uint8_t, 0x01136FA4> _textInputLeft;
    static loco_global<uint8_t, 0x011370A9> _textInputFlags;

    static std::vector<file_entry> _newFiles;

    static void onClose(window* window);
    static void onResize(window* window);
    static void onMouseUp(Ui::window* window, widget_index widgetIndex);
    static void onUpdate(Ui::window* window);
    static void getScrollSize(Ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onScrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index);
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex);
    static void prepareDraw(window* window);
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi);
    static void drawSavePreview(Ui::window& window, Gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height, const S5::SaveDetails& saveInfo);
    static void drawLandscapePreview(Ui::window& window, Gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height);
    static void drawTextInput(Ui::window* window, Gfx::drawpixelinfo_t& dpi, const char* text, int32_t caret, bool showCaret);
    static void drawScroll(Ui::window* window, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void upOneLevel();
    static void appendDirectory(const char* to_append);
    static void processFileForLoadSave(window* window);
    static void processFileForDelete(window* self, file_entry& entry);
    static void refreshDirectoryList();
    static void sub_446E87(window* self);
    static bool filenameContainsInvalidChars();

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
        _events.on_close = onClose;
        _events.on_mouse_up = onMouseUp;
        _events.on_resize = onResize;
        _events.on_update = onUpdate;
        _events.get_scroll_size = getScrollSize;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;

        auto path = fs::path(szPath);
        auto directory = getDirectory(path);
        auto baseName = getBasename(path);

        TextInput::cancel();

        *_type = type;
        *_fileType = browse_file_type::saved_game;
        if (Utility::iequals(filter, S5::filterSC5))
        {
            *_fileType = browse_file_type::landscape;
        }
        Utility::strcpy_safe(_title, title);
        Utility::strcpy_safe(_filter, filter);

        Utility::strcpy_safe(_directory, directory.make_preferred().u8string().c_str());
        Utility::strcpy_safe(_text_input_buffer, baseName.c_str());

        refreshDirectoryList();

        auto window = WindowManager::createWindowCentred(
            WindowType::fileBrowserPrompt,
            { 500, 380 },
            Ui::WindowFlags::stick_to_front | Ui::WindowFlags::resizable | Ui::WindowFlags::flag_12,
            &_events);

        if (window != nullptr)
        {
            window->widgets = widgets;
            window->enabled_widgets = (1 << widx::close_button) | (1 << widx::parent_button) | (1 << widx::ok_button);
            window->initScrollWidgets();

            _textInputCaret = -1;
            _textInputFlags = 0;
            _textInputLeft = 0;

            window->row_height = 11;
            window->var_85A = -1;

            addr<0x009DA285, uint8_t>() = 0;
            TextInput::calculateTextOffset(addr<0x0050ADAC, int16_t>() - addr<0x0050ADAA, int16_t>());

            window->colours[0] = Colour::black;
            window->colours[1] = Colour::saturated_green;

            WindowManager::setCurrentModalType(WindowType::fileBrowserPrompt);
            promptTickLoop(
                []() {
                    Input::handleKeyboard();
                    Audio::updateSounds();
                    WindowManager::dispatchUpdateAll();
                    Input::processKeyboardInput();
                    WindowManager::update();
                    Ui::minimalHandleInput();
                    Gfx::render();
                    return WindowManager::find(WindowType::fileBrowserPrompt) != nullptr;
                });
            WindowManager::setCurrentModalType(WindowType::undefined);
            // TODO: use Utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(szPath, _directory);
            if (szPath[0] != '\0')
            {
                return true;
            }
        }
        return false;
    }

    // 0x0044647C
    static void onClose(window*)
    {
        _newFiles = {};
        _numFiles = 0;
        _files = (file_entry*)-1;

        call(0x00447174);
    }

    // 0x004467F6
    static void onResize(window* window)
    {
        window->capSize(400, 300, 640, 800);
    }

    // 0x00446465
    static void onMouseUp(Ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                _directory[0] = '\0';
                WindowManager::close(window);
                break;
            case widx::parent_button:
                upOneLevel();
                window->invalidate();
                break;
            case widx::ok_button:
                processFileForLoadSave(window);
                break;
        }
    }

    // 0x004467E1
    static void onUpdate(Ui::window* window)
    {
        _textInputFlags++;
        if ((_textInputFlags & 0x0F) == 0)
        {
            window->invalidate();
        }
    }

    // 0x004464A1
    static void getScrollSize(Ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->row_height * _numFiles;
    }

    // 0x004464F7
    static void onScrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scrollIndex)
    {
        auto index = y / self->row_height;
        if (index > _numFiles)
            return;

        Audio::playSound(Audio::sound_id::click_down, self->x + (self->width / 2));

        file_entry entry = _files[index];

        // Clicking a directory, with left mouse button?
        if (Input::state() == Input::input_state::scroll_left && entry.is_directory())
        {
            appendDirectory(entry.get_name().data());
            self->invalidate();
            return;
        }

        // Clicking a file, with left mouse button?
        if (Input::state() == Input::input_state::scroll_left)
        {
            // Copy the selected filename without extension to text input buffer.
            strncpy(_text_input_buffer, entry.get_name().data(), std::size(_text_input_buffer));

            // Continue processing for load/save.
            processFileForLoadSave(self);
        }
        // Clicking a file, with right mouse button
        else
        {
            processFileForDelete(self, entry);
        }
    }

    // 0x004464B1
    static void onScrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scrollIndex)
    {
        if (WindowManager::getCurrentModalType() != WindowType::fileBrowserPrompt)
            return;

        auto index = y / self->row_height;
        if (index >= _numFiles)
            return;

        if (self->var_85A == index)
            return;

        self->var_85A = index;
        sub_446E87(self);
        self->invalidate();
    }

    // 0x004467D7
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x00445C8F
    static void prepareDraw(Ui::window* self)
    {
        // TODO: replace with a fixed length!
        char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
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
        regs.edi = ToInt(buffer);
        regs.esi = ToInt(self);
        call(0x00445D91, regs);
    }

    static void setCommonArgsStringptr(const char* buffer)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;
        auto stringptrId = StringIds::stringptr;
        std::memcpy(_commonFormatArgs, &stringptrId, sizeof(stringptrId));
        std::memcpy(_commonFormatArgs + 2, &buffer, sizeof(buffer));
    }

    // 0x00445E38
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;
        static std::string _nameBuffer;

        window->draw(dpi);

        auto folder = (const char*)0x9DA084;
        setCommonArgsStringptr(folder);
        Gfx::drawString_494B3F(*dpi, window->x + 3, window->y + window->widgets[widx::parent_button].top + 6, 0, StringIds::window_browse_folder, _commonFormatArgs);

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
                setCommonArgsStringptr(_nameBuffer.c_str());
                Gfx::drawStringCentredClipped(
                    *dpi,
                    x + (width / 2),
                    y,
                    width,
                    0,
                    StringIds::wcolour2_stringid,
                    _commonFormatArgs);
                y += 12;

                if (*_fileType == browse_file_type::saved_game)
                {
                    // Preview image
                    auto saveInfo = *((const S5::SaveDetails**)0x50AEA8);
                    if (saveInfo != (void*)-1)
                    {
                        drawSavePreview(*window, *dpi, x, y, width, 201, *saveInfo);
                    }
                }
                else if (*_fileType == browse_file_type::landscape)
                {
                    drawLandscapePreview(*window, *dpi, x, y, width, 129);
                }
            }
        }

        const auto& filenameBox = window->widgets[widx::text_filename];
        if (filenameBox.type != widget_type::none)
        {
            // Draw filename label
            Gfx::drawString_494B3F(*dpi, window->x + 3, window->y + filenameBox.top + 2, 0, StringIds::window_browse_filename, nullptr);

            // Clip to text box
            Gfx::drawpixelinfo_t* dpi2;
            if (Gfx::clipDrawpixelinfo(
                    &dpi2,
                    dpi,
                    window->x + filenameBox.left + 1,
                    window->y + filenameBox.top + 1,
                    filenameBox.right - filenameBox.left - 1,
                    filenameBox.bottom - filenameBox.top - 1))
            {
                drawTextInput(window, *dpi2, _text_input_buffer, _textInputCaret, (_textInputFlags & 0x10) == 0);
            }
        }
    }

    static void drawSavePreview(Ui::window& window, Gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height, const S5::SaveDetails& saveInfo)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;

        Gfx::fillRectInset(&dpi, x, y, x + width, y + height, window.colours[1], 0x30);

        auto imageId = 0;
        auto g1 = Gfx::getG1Element(imageId);
        if (g1 != nullptr)
        {
            // Temporarily substitute a g1 for the image data in the saved game
            auto backupg1 = *g1;
            *g1 = {};
            g1->offset = (uint8_t*)saveInfo.image;
            g1->width = 250;
            g1->height = 200;
            Gfx::drawImage(&dpi, x + 1, y + 1, imageId);
            *g1 = backupg1;
        }
        y += 207;

        uint16_t maxWidth = window.width - window.widgets[widx::scrollview].right;

        // Company
        setCommonArgsStringptr(saveInfo.company);
        y = Gfx::drawString_495224(dpi, x, y, maxWidth, Colour::black, StringIds::window_browse_company, _commonFormatArgs);

        // Owner
        setCommonArgsStringptr(saveInfo.owner);
        y = Gfx::drawString_495224(dpi, x, y, maxWidth, Colour::black, StringIds::owner_label, _commonFormatArgs);

        // Date
        y = Gfx::drawString_495224(dpi, x, y, maxWidth, Colour::black, StringIds::window_browse_date, &saveInfo.date);

        // Challenge progress
        auto flags = saveInfo.challenge_flags;
        if (!(flags & company_flags::challenge_beaten_by_opponent))
        {
            auto stringId = StringIds::window_browse_challenge_completed;
            int16_t progress = 0;
            if (!(flags & company_flags::challenge_completed))
            {
                stringId = StringIds::window_browse_challenge_failed;
                if (!(flags & company_flags::challenge_failed))
                {
                    stringId = StringIds::window_browse_challenge_progress;
                    progress = saveInfo.challenge_progress;
                }
            }
            Gfx::drawString_495224(dpi, x, y, maxWidth, Colour::black, stringId, &progress);
        }
    }

    static void drawLandscapePreview(Ui::window& window, Gfx::drawpixelinfo_t& dpi, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        Gfx::fillRectInset(&dpi, x, y, x + width, y + height, window.colours[1], 0x30);

        if (S5::getPreviewOptions().scenarioFlags & Scenario::flags::landscape_generation_done)
        {
            // Height map
            auto imageId = 0;
            auto g1 = Gfx::getG1Element(imageId);
            if (g1 != nullptr)
            {
                // Temporarily substitute a g1 for the height map image data in the saved game
                auto backupg1 = *g1;
                *g1 = {};
                g1->offset = (uint8_t*)0x9CCBDE;
                g1->width = 128;
                g1->height = 128;
                Gfx::drawImage(&dpi, x + 1, y + 1, imageId);
                *g1 = backupg1;

                Gfx::drawImage(&dpi, x, y + 1, ImageIds::height_map_compass);
            }
        }
        else
        {
            // Randomly generated landscape
            auto imageId = ImageIds::random_map_watermark | (window.colours[1] << 19) | 0x20000000;
            Gfx::drawImage(&dpi, x, y, imageId);
            Gfx::point_t origin = { (int16_t)(x + 64), (int16_t)(y + 60) };
            Gfx::drawStringCentredWrapped(&dpi, &origin, 128, 0, StringIds::randomly_generated_landscape);
        }
    }

    static void drawTextInput(Ui::window* window, Gfx::drawpixelinfo_t& dpi, const char* text, int32_t caret, bool showCaret)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;
        loco_global<uint8_t[256], 0x001136C99> byte_1136C99;
        static std::string gbuffer;

        // Draw text box text
        Gfx::point_t origin = { 0, 1 };
        setCommonArgsStringptr(text);
        Gfx::drawString_494B3F(dpi, &origin, 0, StringIds::black_stringid, _commonFormatArgs);

        if (showCaret)
        {
            if (caret == -1)
            {
                // Draw horizontal caret
                Gfx::drawString_494B3F(dpi, &origin, 0, StringIds::window_browse_input_caret, nullptr);
            }
            else
            {
                // Draw text[0:caret] over the top
                // TODO this should really just be measuring the string
                gbuffer = std::string_view(text, caret);
                setCommonArgsStringptr(gbuffer.c_str());
                origin = { 0, 1 };
                Gfx::drawString_494B3F(dpi, &origin, 0, StringIds::black_stringid, _commonFormatArgs);

                // Draw vertical caret
                Gfx::drawRect(&dpi, origin.x, origin.y, 1, 9, byte_1136C99[window->colours[1] * 8]);
            }
        }
    }

    // 0x00446314
    static void drawScroll(Ui::window* window, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        loco_global<char[16], 0x0112C826> _commonFormatArgs;

        static std::string _nameBuffer;

        // Background
        Gfx::clearSingle(*dpi, Colour::getShade(window->colours[1], 4));

        // Directories / files
        auto y = 0;
        auto lineHeight = window->row_height;
        for (auto i = 0; i < _numFiles; i++)
        {
            if (y + lineHeight >= dpi->y && y <= dpi->y + dpi->height)
            {
                auto file = _files[i];

                // Draw the row highlight
                auto stringId = StringIds::black_stringid;
                if (i == window->var_85A)
                {
                    Gfx::drawRect(dpi, 0, y, window->width, lineHeight, 0x2000000 | 48);
                    stringId = StringIds::wcolour2_stringid;
                }

                // Draw the folder icon
                auto x = 1;
                if (file.is_directory())
                {
                    Gfx::drawImage(dpi, x, y, ImageIds::icon_folder);
                    x += 14;
                }

                // Copy name to our work buffer
                _nameBuffer = file.get_name();

                // Draw the name
                setCommonArgsStringptr(_nameBuffer.c_str());
                Gfx::drawString_494B3F(*dpi, x, y, 0, stringId, _commonFormatArgs);
            }
            y += lineHeight;
        }
    }

    static fs::path getDirectory(const fs::path& path)
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
            return str / fs::path();
        }
    }

    static std::string getBasename(const fs::path& path)
    {
        auto baseName = path.stem().u8string();
        if (baseName == ".")
        {
            baseName = "";
        }
        return baseName;
    }

    // 0x00446A93
    static void refreshDirectoryList()
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
                try
                {
                    for (auto& f : fs::directory_iterator(directory))
                    {
                        bool isDirectory = f.is_directory();
                        if (f.is_regular_file())
                        {
                            auto extension = f.path().extension().u8string();
                            if (!Utility::iequals(extension, filterExtension))
                            {
                                continue;
                            }
                        }
                        else if (!isDirectory) // Only list directories or normal files
                        {
                            continue;
                        }
                        auto name = f.path().stem().u8string();
                        _newFiles.emplace_back(name, isDirectory);
                    }
                }
                catch (const fs::filesystem_error& err)
                {
                    Console::error("Invalid directory or file: %s", err.what());
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
    static void upOneLevel()
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

        refreshDirectoryList();
    }

    // 0x00446E62
    static void appendDirectory(const char* to_append)
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

        refreshDirectoryList();
    }

    void registerHooks()
    {
        registerHook(
            0x00445AB9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto result = open(
                    (browse_type)regs.al,
                    ToPtr(char, regs.ecx),
                    ToPtr(const char, regs.edx),
                    ToPtr(const char, regs.ebx));
                regs.eax = result ? 1 : 0;
                return 0;
            });

        registerHook(
            0x00446E62,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                appendDirectory(ToPtr(char, regs.ebp));
                return 0;
            });
    }

    // 0x00446F1D
    static bool filenameContainsInvalidChars()
    {
        uint8_t numNonSpacesProcessed = 0;
        for (const char* ptr = &*_text_input_buffer; *ptr != '\0'; ptr++)
        {
            if (*ptr != ' ')
                numNonSpacesProcessed++;

            switch (*ptr)
            {
                // The following chars are considered invalid in filenames.
                case '.':
                case '"':
                case '\\':
                case '*':
                case '?':
                case ':':
                case ';':
                case ',':
                case '<':
                case '>':
                case '/':
                    return true;
            }
        }

        // If we have only processed spaces, the filename is invalid as well.
        return numNonSpacesProcessed == 0;
    }

    static constexpr const char* getExtensionFromFileType(browse_file_type type)
    {
        switch (type)
        {
            case browse_file_type::saved_game:
                return S5::extensionSV5;
            case browse_file_type::landscape:
            default:
                return S5::extensionSC5;
        }
    }

    // 0x00446574
    static void processFileForLoadSave(window* self)
    {
        if (_type == browse_type::save)
        {
            if (filenameContainsInvalidChars())
            {
                Windows::showError(StringIds::error_invalid_filename);
                return;
            }

            // Create full path to target file.
            fs::path path = fs::path(&_directory[0]) / std::string(&_text_input_buffer[0]);

            // Append extension to filename.
            path += getExtensionFromFileType(_fileType);

            // Does the file already exist?
            if (fs::exists(path))
            {
                // Copy directory and filename to buffer.
                char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
                strncpy(&buffer_2039[0], &_text_input_buffer[0], 512);

                FormatArguments args{};
                args.push(StringIds::buffer_2039);

                // Copy window title into title buffer for ok/cancel window.
                strncpy(&_stringFormatBuffer[0], _title, 256);

                // Formatted string into description buffer for ok/cancel window.
                loco_global<char[512], 0x0112CE04> descriptionBuffer;
                StringManager::formatString(&descriptionBuffer[0], StringIds::replace_existing_file_prompt, &args);

                // Ask for confirmation to replace the file.
                if (!Windows::promptOkCancel(StringIds::replace_existing_file_button))
                    return;
            }

            // Copy directory and filename to buffer.
            strncpy(&_directory[0], path.u8string().c_str(), std::size(_directory));

            // Close browse window to continue saving.
            WindowManager::close(self);
        }
        else
        {
            // Copy scenario path into expected address. Trailing / on directory is assumed.
            // TODO: refactor to fs::path?
            strncat(_directory, _text_input_buffer, std::size(_directory));
            strncat(_directory, getExtensionFromFileType(_fileType), std::size(_directory));

            // Close browse window to start loading.
            WindowManager::close(self);
        }
    }

    // 0x004466CA
    static void processFileForDelete(window* self, file_entry& entry)
    {
        // Create full path to target file.
        fs::path path = fs::path(&_directory[0]) / std::string(entry.get_name());

        // Append extension to filename.
        path += getExtensionFromFileType(_fileType);

        // Copy directory and filename to buffer.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(&buffer_2039[0], entry.get_name().data(), 512);

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        // Copy window title into title buffer for ok/cancel window.
        strncpy(&_stringFormatBuffer[0], _title, 256);

        // Formatted string into description buffer for ok/cancel window.
        loco_global<char[512], 0x0112CE04> descriptionBuffer;
        StringManager::formatString(&descriptionBuffer[0], StringIds::delete_file_prompt, &args);

        // Ask for confirmation to delete the file.
        if (!Windows::promptOkCancel(StringIds::delete_file_button))
            return;

        // Actually remove the file..!
        fs::remove(path);

        // Refresh window
        refreshDirectoryList();
        self->invalidate();
    }

    // 0x00446E87
    // TODO: only called by this window -- implement.
    static void sub_446E87(window* self)
    {
        registers regs;
        regs.esi = ToInt(self);
        call(0x00446E87, regs);
    }
}
