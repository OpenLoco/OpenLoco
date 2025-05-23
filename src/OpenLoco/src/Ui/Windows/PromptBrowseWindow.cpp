#include "Audio/Audio.h"
#include "Config.h"
#include "Environment.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/Conversion.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "ScenarioOptions.h"
#include "Ui.h"
#include "Ui/TextInput.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Utility/String.hpp>

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstring>
#include <string>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Ui::Windows::PromptBrowse
{
    enum BrowseFileType : uint8_t
    {
        savedGame,
        landscape,
        heightmap,
    };

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

    static constexpr auto widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 500, 380 }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 498, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::empty),
        Widgets::ImageButton({ 485, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 500, 365 }, WindowColour::secondary),
        Widgets::ImageButton({ 473, 18 }, { 24, 24 }, WindowColour::secondary, ImageIds::icon_parent_folder, StringIds::window_browse_parent_folder_tooltip),
        Widgets::TextBox({ 88, 348 }, { 408, 14 }, WindowColour::secondary),
        Widgets::Button({ 426, 364 }, { 70, 12 }, WindowColour::secondary, StringIds::label_button_ok),
        Widgets::ScrollView({ 3, 45 }, { 494, 323 }, WindowColour::secondary, Scrollbars::vertical)

    );

    static loco_global<uint8_t, 0x009D9D63> _type;
    static loco_global<BrowseFileType, 0x009DA284> _fileType;
    static loco_global<char[512], 0x009DA084> _displayFolderBuffer;
    static loco_global<char[32], 0x009D9E64> _filter;
    static loco_global<char[512], 0x0112CE04> _savePath;

    // 0x0050AEA8
    static std::unique_ptr<S5::SaveDetails> _previewSaveDetails;
    // 0x009CCA54
    static std::unique_ptr<Scenario::Options> _previewScenarioOptions;

    static Ui::TextInput::InputSession inputSession;

    static fs::path _currentDirectory;
    static std::vector<fs::path> _files;

    static fs::path getDirectory(const fs::path& path);
    static std::string getBasename(const fs::path& path);

    static void drawSavePreview(Ui::Window& window, Gfx::DrawingContext& drawingCtx, int32_t x, int32_t y, int32_t width, int32_t height, const S5::SaveDetails& saveInfo);
    static void drawLandscapePreview(Ui::Window& window, Gfx::DrawingContext& drawingCtx, int32_t x, int32_t y, int32_t width, int32_t height);
    static void drawTextInput(Ui::Window* window, Gfx::DrawingContext& drawingCtx, const char* text, int32_t caret, bool showCaret);
    static void upOneLevel();
    static void changeDirectory(const fs::path& path);
    static void processFileForLoadSave(Window* window);
    static void processFileForDelete(Window* self, fs::path& entry);
    static void refreshDirectoryList();
    static void loadFileDetails(Window* self);
    static bool filenameContainsInvalidChars();
    static const WindowEventList& getEvents();

    // 0x00445AB9
    // ecx: path
    // edx: filter
    // ebx: title
    // eax: {return}
    bool open(
        browse_type type,
        char* szPath,
        const char* filter,
        StringId titleId)
    {
        auto path = fs::u8path(szPath);
        auto directory = getDirectory(path);
        auto baseName = getBasename(path);

        TextInput::cancel();

        *_type = type;
        *_fileType = BrowseFileType::savedGame;
        if (Utility::iequals(filter, S5::filterSC5))
        {
            *_fileType = BrowseFileType::landscape;
        }
        // TODO: make named constant for filter
        else if (Utility::iequals(filter, "*.png"))
        {
            *_fileType = BrowseFileType::heightmap;
        }
        Utility::strlcpy(_filter, filter, _filter.size());

        changeDirectory(directory.make_preferred());
        inputSession = Ui::TextInput::InputSession(baseName, 200);

        auto window = WindowManager::createWindowCentred(
            WindowType::fileBrowserPrompt,
            { 500, 380 },
            Ui::WindowFlags::stickToFront | Ui::WindowFlags::resizable | Ui::WindowFlags::flag_12,
            getEvents());

        if (window != nullptr)
        {
            window->setWidgets(widgets);
            window->widgets[widx::caption].text = titleId;
            window->initScrollWidgets();

            window->rowHeight = 11;
            window->var_85A = -1;

            auto& widget = window->widgets[widx::text_filename];
            inputSession.calculateTextOffset(widget.width());

            // Focus the textbox element
            Input::setFocus(window->type, window->number, widx::text_filename);

            window->setColour(WindowColour::primary, Colour::black);
            window->setColour(WindowColour::secondary, Colour::mutedSeaGreen);

            WindowManager::setCurrentModalType(WindowType::fileBrowserPrompt);
            const bool success = promptTickLoop(
                []() {
                    Input::handleKeyboard();
                    Audio::updateSounds();
                    WindowManager::dispatchUpdateAll();
                    Input::processKeyboardInput();
                    Input::processMouseWheel();
                    WindowManager::update();
                    Ui::minimalHandleInput();
                    Gfx::renderAndUpdate();
                    return WindowManager::find(WindowType::fileBrowserPrompt) != nullptr;
                });
            WindowManager::setCurrentModalType(WindowType::undefined);

            // TODO: return std::optional instead
            return success && _savePath[0] != '\0';
        }
        return false;
    }

    // 0x00447174
    static void freeFileDetails()
    {
        _previewSaveDetails.reset();
    }

    // 0x0044647C
    static void onClose(Window&)
    {
        _files.clear();
        freeFileDetails();
    }

    // 0x004467F6
    static void onResize(Window& window)
    {
        window.capSize(400, 300, 640, 800);
    }

    // 0x00446465
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                _currentDirectory.clear();
                _savePath[0] = '\0';
                WindowManager::close(&window);
                break;
            case widx::parent_button:
                upOneLevel();
                window.var_85A = -1;
                window.initScrollWidgets();
                window.invalidate();
                break;
            case widx::ok_button:
                processFileForLoadSave(&window);
                break;
        }
    }

    // 0x004467E1
    static void onUpdate(Ui::Window& window)
    {
        inputSession.cursorFrame++;
        if ((inputSession.cursorFrame & 0x0F) == 0)
        {
            window.invalidate();
        }
    }

    // 0x004464A1
    static void getScrollSize(Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window.rowHeight * static_cast<uint16_t>(_files.size());
    }

    // 0x004464F7
    static void onScrollMouseDown(Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
    {
        auto index = size_t(y / self.rowHeight);
        if (index >= _files.size())
        {
            return;
        }

        Audio::playSound(Audio::SoundId::clickDown, self.x + (self.width / 2));

        auto& entry = _files[index];

        // Clicking a directory, with left mouse button?
        if (Input::state() == Input::State::scrollLeft && fs::is_directory(entry))
        {
            changeDirectory(entry);
            self.var_85A = -1;
            self.initScrollWidgets();
            self.invalidate();
            return;
        }

        // Clicking a file, with left mouse button?
        if (Input::state() == Input::State::scrollLeft)
        {
            // Copy the selected filename without extension to text input buffer.
            inputSession.buffer = entry.stem().u8string();
            inputSession.cursorPosition = inputSession.buffer.length();
            self.invalidate();

            // Continue processing for load/save.
            processFileForLoadSave(&self);
        }
        // Clicking a file, with right mouse button
        else
        {
            processFileForDelete(&self, entry);
        }
    }

    // 0x004464B1
    static void onScrollMouseOver(Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
    {
        if (WindowManager::getCurrentModalType() != WindowType::fileBrowserPrompt)
        {
            return;
        }

        auto index = y / self.rowHeight;
        if (index >= static_cast<uint16_t>(_files.size()))
        {
            return;
        }

        if (self.var_85A == index)
        {
            return;
        }

        self.var_85A = index;
        loadFileDetails(&self);
        self.invalidate();
    }

    // 0x004467D7
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x00445C8F
    static void prepareDraw(Ui::Window& self)
    {
        self.widgets[widx::frame].right = self.width - 1;
        self.widgets[widx::frame].bottom = self.height - 1;

        self.widgets[widx::panel].right = self.width - 1;
        self.widgets[widx::panel].bottom = self.height - 1;

        self.widgets[widx::caption].right = self.width - 2;

        self.widgets[widx::close_button].left = self.width - 15;
        self.widgets[widx::close_button].right = self.width - 3;

        if (*_type == browse_type::save)
        {
            self.widgets[widx::ok_button].left = self.width - 86;
            self.widgets[widx::ok_button].right = self.width - 16;
            self.widgets[widx::ok_button].top = self.height - 15;
            self.widgets[widx::ok_button].bottom = self.height - 4;
            self.widgets[widx::ok_button].hidden = false;

            self.widgets[widx::text_filename].right = self.width - 4;
            self.widgets[widx::text_filename].top = self.height - 31;
            self.widgets[widx::text_filename].bottom = self.height - 18;
            self.widgets[widx::text_filename].hidden = false;

            self.widgets[widx::scrollview].bottom = self.height - 34;
        }
        else
        {
            self.widgets[widx::ok_button].hidden = true;
            self.widgets[widx::text_filename].hidden = true;

            self.widgets[widx::scrollview].bottom = self.height - 4;
        }

        self.widgets[widx::scrollview].right = self.width - 259;
        if (*_fileType != BrowseFileType::savedGame)
        {
            self.widgets[widx::scrollview].right += 122;
        }

        self.widgets[widx::parent_button].left = self.width - 26;
        self.widgets[widx::parent_button].right = self.width - 3;

        // Get width of the base 'Folder:' string
        char folderBuffer[256]{};
        FormatArguments args{};
        args.push(StringIds::empty);
        StringManager::formatString(folderBuffer, StringIds::window_browse_folder, args);

        const auto folderLabelWidth = Gfx::TextRenderer::getStringWidth(Gfx::Font::medium_bold, folderBuffer);

        // We'll ensure the folder width does not reach the parent button.
        const uint16_t maxWidth = self.widgets[widx::parent_button].left - folderLabelWidth - 10;
        auto nameBuffer = _currentDirectory.u8string();
        nameBuffer = Localisation::convertUnicodeToLoco(nameBuffer);
        strncpy(&_displayFolderBuffer[0], nameBuffer.c_str(), 512);
        uint16_t folderWidth = Gfx::TextRenderer::getStringWidth(Gfx::Font::medium_bold, _displayFolderBuffer);

        // If the folder already fits, we're done.
        if (folderWidth <= maxWidth)
        {
            return;
        }

        const char* relativeDirectory = nameBuffer.c_str();
        do
        {
            // If we're omitting part of the folder, prepend ellipses.
            if (relativeDirectory != nameBuffer.c_str())
            {
                strncpy(&_displayFolderBuffer[0], "...", 512);
            }

            // Seek the next directory separator token.
            while (*relativeDirectory != '\0' && *relativeDirectory != fs::path::preferred_separator)
            {
                relativeDirectory++;
            }

            // Use the truncated directory name in the buffer.
            strncat(&_displayFolderBuffer[0], relativeDirectory, 512);

            // Prepare for the next pass, if needed.
            relativeDirectory++;
        } while (Gfx::TextRenderer::getStringWidth(Gfx::Font::medium_bold, _displayFolderBuffer) > maxWidth);
    }

    static FormatArguments getStringPtrFormatArgs(const char* buffer)
    {
        FormatArguments args = {};
        args.push(StringIds::stringptr);
        args.push(buffer);
        return args;
    }

    // 0x00445E38
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        window.draw(drawingCtx);

        {
            auto folder = &_displayFolderBuffer[0];
            auto args = getStringPtrFormatArgs(folder);
            auto point = Point(3, window.widgets[widx::parent_button].top + 6);
            tr.drawStringLeft(point, Colour::black, StringIds::window_browse_folder, args);
        }

        auto selectedIndex = window.var_85A;
        if (selectedIndex != -1)
        {
            auto& selectedFile = _files[selectedIndex];
            if (!fs::is_directory(selectedFile))
            {
                const auto& widget = window.widgets[widx::scrollview];

                auto width = window.width - widget.right - 8;
                auto x = widget.right + 3;
                auto y = 45;

                auto nameBuffer = selectedFile.stem().u8string();
                nameBuffer = Localisation::convertUnicodeToLoco(nameBuffer);

                auto args = getStringPtrFormatArgs(nameBuffer.c_str());
                auto point = Point(x + (width / 2), y);
                tr.drawStringCentredClipped(
                    point,
                    width,
                    Colour::black,
                    StringIds::wcolour2_stringid,
                    args);
                y += 12;

                if (*_fileType == BrowseFileType::savedGame)
                {
                    // Preview image
                    if (_previewSaveDetails != nullptr)
                    {
                        drawSavePreview(window, drawingCtx, x, y, width, 201, *_previewSaveDetails);
                    }
                }
                else if (*_fileType == BrowseFileType::landscape)
                {
                    if (_previewScenarioOptions != nullptr)
                    {
                        drawLandscapePreview(window, drawingCtx, x, y, width, 129);
                    }
                }
            }
        }

        const auto& filenameBox = window.widgets[widx::text_filename];
        if (!filenameBox.hidden)
        {
            // Draw filename label
            auto point = Point(3, filenameBox.top + 2);
            tr.drawStringLeft(point, Colour::black, StringIds::window_browse_filename);

            // Clip to text box
            const auto& rt = drawingCtx.currentRenderTarget();
            auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(filenameBox.left + 1, filenameBox.top + 1, filenameBox.right - filenameBox.left - 1, filenameBox.bottom - filenameBox.top - 1));
            if (clipped)
            {
                drawingCtx.pushRenderTarget(*clipped);

                bool showCaret = Input::isFocused(window.type, window.number, widx::text_filename) && (inputSession.cursorFrame & 0x10) == 0;
                drawTextInput(&window, drawingCtx, inputSession.buffer.c_str(), inputSession.cursorPosition, showCaret);

                drawingCtx.popRenderTarget();
            }
        }
    }

    static void drawSavePreview(Ui::Window& window, Gfx::DrawingContext& drawingCtx, int32_t x, int32_t y, int32_t width, int32_t height, const S5::SaveDetails& saveInfo)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.fillRectInset(x, y, x + width, y + height, window.getColour(WindowColour::secondary), Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillNone);

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
            drawingCtx.drawImage(x + 1, y + 1, imageId);
            *g1 = backupg1;
        }
        y += 207;

        uint16_t maxWidth = window.width - window.widgets[widx::scrollview].right;
        auto point = Point(x, y);

        // Company
        {
            auto args = getStringPtrFormatArgs(saveInfo.company);
            point = tr.drawStringLeftWrapped(point, maxWidth, Colour::black, StringIds::window_browse_company, args);
        }

        // Owner
        {
            auto args = getStringPtrFormatArgs(saveInfo.owner);
            point = tr.drawStringLeftWrapped(point, maxWidth, Colour::black, StringIds::owner_label, args);
        }

        // Date
        {
            auto argsBuf = FormatArgumentsBuffer{};
            auto args = FormatArguments{ argsBuf };
            args.push(saveInfo.date);
            point = tr.drawStringLeftWrapped(point, maxWidth, Colour::black, StringIds::window_browse_date, args);
        }

        // Challenge progress
        auto flags = saveInfo.challengeFlags;
        if ((flags & CompanyFlags::challengeBeatenByOpponent) == CompanyFlags::none)
        {
            auto stringId = StringIds::window_browse_challenge_completed;
            int16_t progress = 0;
            if ((flags & CompanyFlags::challengeCompleted) == CompanyFlags::none)
            {
                stringId = StringIds::window_browse_challenge_failed;
                if ((flags & CompanyFlags::challengeFailed) == CompanyFlags::none)
                {
                    stringId = StringIds::window_browse_challenge_progress;
                    progress = saveInfo.challengeProgress;
                }
            }

            auto argsBuf = FormatArgumentsBuffer{};
            auto args = FormatArguments{ argsBuf };
            args.push(progress);
            tr.drawStringLeftWrapped(point, maxWidth, Colour::black, stringId, args);
        }
    }

    static void drawLandscapePreview(Ui::Window& window, Gfx::DrawingContext& drawingCtx, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.fillRectInset(x, y, x + width, y + height, window.getColour(WindowColour::secondary), Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillNone);

        if ((_previewScenarioOptions->scenarioFlags & Scenario::ScenarioFlags::landscapeGenerationDone) != Scenario::ScenarioFlags::none)
        {
            // Height map
            auto imageId = 0;
            auto g1 = Gfx::getG1Element(imageId);
            if (g1 != nullptr)
            {
                // Temporarily substitute a g1 for the height map image data in the saved game
                auto backupg1 = *g1;
                *g1 = {};
                g1->offset = &_previewScenarioOptions->preview[0][0];
                g1->width = 128;
                g1->height = 128;
                drawingCtx.drawImage(x + 1, y + 1, imageId);
                *g1 = backupg1;

                drawingCtx.drawImage(x, y + 1, ImageIds::height_map_compass);
            }
        }
        else
        {
            // Randomly generated landscape
            auto imageId = Gfx::recolour(ImageIds::random_map_watermark, window.getColour(WindowColour::secondary).c());
            drawingCtx.drawImage(x, y, imageId);
            auto origin = Ui::Point(x + 64, y + 60);
            tr.drawStringCentredWrapped(origin, 128, Colour::black, StringIds::randomly_generated_landscape);
        }
    }

    static void drawTextInput(Ui::Window* window, Gfx::DrawingContext& drawingCtx, const char* text, int32_t caret, bool showCaret)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw text box text
        Ui::Point origin = { 0, 1 };
        {
            auto args = getStringPtrFormatArgs(text);
            tr.drawStringLeft(origin, Colour::black, StringIds::black_stringid, args);
        }

        if (showCaret)
        {
            if (caret == -1)
            {
                // Draw horizontal caret
                tr.drawStringLeft(origin, Colour::black, StringIds::window_browse_input_caret);
            }
            else
            {
                // Draw text[0:caret] over the top
                // TODO this should really just be measuring the string
                const std::string gbuffer = std::string(text, caret);
                auto args = getStringPtrFormatArgs(gbuffer.c_str());
                origin = { 0, 1 };
                origin = tr.drawStringLeft(origin, Colour::black, StringIds::black_stringid, args);

                // Draw vertical caret
                drawingCtx.drawRect(origin.x, origin.y, 1, 9, Colours::getShade(window->getColour(WindowColour::secondary).c(), 9), Gfx::RectFlags::none);
            }
        }
    }

    static bool isRootPath(const fs::path& entry)
    {
        return (entry == entry.root_path());
    }

    // 0x00446314
    static void drawScroll(Ui::Window& window, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Background
        drawingCtx.clearSingle(Colours::getShade(window.getColour(WindowColour::secondary).c(), 4));

        // Directories / files
        auto i = 0;
        auto y = 0;
        auto lineHeight = window.rowHeight;
        for (const auto& entry : _files)
        {
            if (y + lineHeight < rt.y)
            {
                y += lineHeight;
                i++;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            // Draw the row highlight
            auto stringId = StringIds::black_stringid;
            if (i == window.var_85A)
            {
                drawingCtx.drawRect(0, y, window.width, lineHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                stringId = StringIds::wcolour2_stringid;
            }

            // Draw the folder icon (TODO: draw a drive for rootPath)
            auto x = 1;
            if (isRootPath(entry) || fs::is_directory(entry))
            {
                drawingCtx.drawImage(x, y, ImageIds::icon_folder);
                x += 14;
            }

            // Copy name to our work buffer (if drive letter use the full path)
            auto nameBuffer = isRootPath(entry) ? entry.u8string() : entry.stem().u8string();
            nameBuffer = Localisation::convertUnicodeToLoco(nameBuffer);

            // Draw the name
            auto args = getStringPtrFormatArgs(nameBuffer.c_str());
            auto point = Point(x, y);
            tr.drawStringLeft(point, Colour::black, stringId, args);

            y += lineHeight;
            i++;
        }
    }

    // 0x0044685C
    static bool keyUp(Window& w, uint32_t charCode, uint32_t keyCode)
    {
        if (keyCode == SDLK_RETURN)
        {
            w.callOnMouseUp(widx::ok_button, w.widgets[widx::ok_button].id);
            return true;
        }
        else if (keyCode == SDLK_ESCAPE)
        {
            w.callOnMouseUp(widx::close_button, w.widgets[widx::close_button].id);
            return true;
        }
        else if (!Input::isFocused(w.type, w.number, widx::text_filename) || !inputSession.handleInput(charCode, keyCode))
        {
            return false;
        }

        // 0x00446A6E
        auto containerWidth = w.widgets[widx::text_filename].width() - 2;
        if (inputSession.needsReoffsetting(containerWidth))
        {
            inputSession.calculateTextOffset(containerWidth);
        }
        w.invalidate();

        return true;
    }

    static fs::path getDirectory(const fs::path& path)
    {
        if (path.has_extension())
        {
            return path.parent_path() / "";
        }
        else
        {
            auto str = path.u8string();
            if (str.size() > 0)
            {
                auto lastCharacter = str[str.size() - 1];
                if (lastCharacter == fs::path::preferred_separator)
                {
                    return path;
                }
            }
            return path / "";
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

        _files.clear();
        if (_currentDirectory.empty())
        {
            // Get all drives
            _files = Platform::getDrives();
            return; // no need to sort these as they are already sorted
        }
        else
        {
            try
            {
                for (const auto& file : fs::directory_iterator(_currentDirectory, fs::directory_options::skip_permission_denied))
                {
                    // Only list directories and normal files
                    if (!(file.is_regular_file() || file.is_directory()))
                    {
                        continue;
                    }

                    // Filter files by extension
                    if (file.is_regular_file())
                    {
                        auto extension = file.path().extension().u8string();
                        if (!Utility::iequals(extension, filterExtension))
                        {
                            continue;
                        }
                    }

                    _files.emplace_back(file.path());
                }
            }
            catch (const fs::filesystem_error& err)
            {
                Logging::error("Invalid directory or file: {}", err.what());
            }
        }

        std::sort(_files.begin(), _files.end(), [](const fs::path& a, const fs::path& b) -> bool {
            if (!fs::is_directory(a) && fs::is_directory(b))
            {
                return false;
            }
            if (fs::is_directory(a) && !fs::is_directory(b))
            {
                return true;
            }
            return a.stem() < b.stem();
        });
    }

    // 0x00446E2F
    static void upOneLevel()
    {
#ifdef _WIN32
        // Showing drive letters?
        if (_currentDirectory.empty())
        {
            return;
        }

        // The drive letter level is above file system root level.
        if (isRootPath(_currentDirectory))
        {
            _currentDirectory.clear();
            refreshDirectoryList();
        }
#endif
        // Going up one level (compensating for trailing slashes).
        changeDirectory(_currentDirectory.parent_path().parent_path());
    }

    // 0x00446E62
    static void changeDirectory(const fs::path& newDir)
    {
        _currentDirectory = newDir / "";
        refreshDirectoryList();
    }

    // 0x00446F1D
    static bool filenameContainsInvalidChars()
    {
        uint8_t numNonSpacesProcessed = 0;
        for (const char chr : inputSession.buffer)
        {
            if (chr != ' ')
            {
                numNonSpacesProcessed++;
            }

            switch (chr)
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

    static constexpr const char* getExtensionFromFileType(BrowseFileType type)
    {
        switch (type)
        {
            case BrowseFileType::savedGame:
                return S5::extensionSV5;
            case BrowseFileType::heightmap:
                return ".png";
            case BrowseFileType::landscape:
            default:
                return S5::extensionSC5;
        }
    }

    // 0x00446574
    static void processFileForLoadSave(Window* self)
    {
        // Create full path to target file.
        fs::path path = _currentDirectory / inputSession.buffer;

        // Append extension to filename.
        path += getExtensionFromFileType(_fileType);

        if (_type == browse_type::save)
        {
            if (filenameContainsInvalidChars())
            {
                Error::open(StringIds::error_invalid_filename);
                return;
            }

            // Does the file already exist?
            if (fs::exists(path))
            {
                // Copy directory and filename to buffer.
                char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
                strncpy(&buffer_2039[0], inputSession.buffer.c_str(), 512);

                // Arguments for description text in ok/cancel window.
                FormatArguments args{};
                args.push(StringIds::buffer_2039);

                // Ask for confirmation to replace the file.
                auto titleId = self->widgets[widx::caption].text;
                if (!Windows::PromptOkCancel::open(titleId, StringIds::replace_existing_file_prompt, args, StringIds::replace_existing_file_button))
                {
                    return;
                }
            }
        }

        // Copy directory and filename to buffer.
        strncpy(_savePath.get(), path.u8string().c_str(), std::size(_savePath));

        // Remember the current path for saved games
        if (_fileType == BrowseFileType::savedGame)
        {
            if (!fs::is_directory(_currentDirectory))
            {
                Config::get().lastSavePath = _currentDirectory.parent_path().u8string();
            }
            else
            {
                Config::get().lastSavePath = _currentDirectory.u8string();
            }
            Config::write();
            Environment::resolvePaths();
        }
        // ... and similarly for landscapes
        else if (_fileType == BrowseFileType::landscape)
        {
            if (!fs::is_directory(_currentDirectory))
            {
                Config::get().lastLandscapePath = _currentDirectory.parent_path().u8string();
            }
            else
            {
                Config::get().lastLandscapePath = _currentDirectory.u8string();
            }
            Config::write();
            Environment::resolvePaths();
        }

        // Close browse window to continue saving.
        WindowManager::close(self);
    }

    // 0x004466CA
    static void processFileForDelete(Window* self, fs::path& entry)
    {
        // Create full path to target file.
        fs::path path = _currentDirectory / entry.stem();
        path += getExtensionFromFileType(_fileType);

        // Copy directory and filename to buffer.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(&buffer_2039[0], entry.stem().u8string().c_str(), 512);

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        // Ask for confirmation to delete the file.
        auto titleId = self->widgets[widx::caption].text;
        if (!Windows::PromptOkCancel::open(titleId, StringIds::delete_file_prompt, args, StringIds::delete_file_button))
        {
            return;
        }

        // Actually remove the file..!
        fs::remove(path);

        // Refresh window
        refreshDirectoryList();
        self->invalidate();
    }

    // 0x00446E87
    static void loadFileDetails(Window* self)
    {
        freeFileDetails();

        if (self->var_85A == -1)
        {
            return;
        }

        auto& entry = _files[self->var_85A];
        if (fs::is_directory(entry))
        {
            return;
        }

        // Create full path to target file.
        auto path = _currentDirectory / entry.stem();
        path += getExtensionFromFileType(_fileType);

        // Load save game or scenario info.
        switch (_fileType)
        {
            case BrowseFileType::savedGame:
                _previewSaveDetails = S5::readSaveDetails(path);
                break;
            case BrowseFileType::landscape:
                _previewScenarioOptions = S5::readScenarioOptions(path);
                break;
            default:
                break;
        }
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onResize = onResize,
        .onUpdate = onUpdate,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = onScrollMouseDown,
        .scrollMouseOver = onScrollMouseOver,
        .tooltip = tooltip,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .drawScroll = drawScroll,
        .keyUp = keyUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
