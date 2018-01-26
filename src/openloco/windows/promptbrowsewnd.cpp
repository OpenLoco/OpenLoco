#include <algorithm>
#include <cstring>
#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif
#include "../graphics/colours.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../utility/string.hpp"
#include "../windowmgr.h"

using namespace openloco::interop;

#ifdef _OPENLOCO_USE_BOOST_FS_
namespace fs = boost::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif

namespace openloco::ui::windows
{
    static fs::path get_directory(const fs::path& path);
    static std::string get_basename(const fs::path& path);

    enum class browse_file_type
    {
        saved_game,
        landscape
    };

    loco_global<uint8_t, 0x009D9D63> _type;
    loco_global<uint8_t, 0x009DA284> _fileType;
    loco_global<char[256], 0x009D9D64> _title;
    loco_global<char[32], 0x009D9E64> _filter;
    loco_global<char[512], 0x009D9E84> _directory;
    loco_global<char[512], 0x011369A0> _text_input_buffer;

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
    bool prompt_browse(
        browse_type type,
        char* szPath,
        const char* filter,
        const char* title)
    {
        auto path = fs::path(szPath);
        auto directory = get_directory(path);
        auto baseName = get_basename(path);

        textinput::close();

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
        auto window = windowmgr::create_window_centred(window_type::prompt_browse, 500, 380, 0x1202, (void*)0x004FB308);
        if (window != nullptr)
        {
            window->widgets = (widget*)0x0050AD58;
            window->enabled_widgets = 4 | 0x10 | 0x40;
            window->sub_4CA17F();
            addr<0x01136FA2, int16_t>() = -1;
            addr<0x011370A9, uint8_t>() = 0;
            addr<0x01136FA4, int16_t>() = 0;
            window->var_83E = 11;
            window->var_85A = 0xFFFF;
            addr<0x009DA285, uint8_t>() = 0;
            sub_4CEB67(addr<0x0050ADAC, int16_t>() - addr<0x0050ADAA, int16_t>());
            window->colours[0] = colour::black;
            window->colours[1] = colour::saturated_green;
            windowmgr::current_modal_type(window_type::prompt_browse);
            prompt_tick_loop(
                []() {
                    input::handle_keyboard();
                    sub_48A18C();
                    call(0x004CD3D0);
                    call(0x004BEC5B);
                    windowmgr::update();
                    call(0x004C98CF);
                    call(0x004CF63B);
                    return windowmgr::find(window_type::prompt_browse) != nullptr;
                });
            windowmgr::current_modal_type(window_type::undefined);
            // TODO: use utility::strlcpy with the buffer size instead of std::strcpy, if possible
            std::strcpy(szPath, _directory);
            if (szPath[0] != '\0')
            {
                return true;
            }
        }
        return false;
    }

    static fs::path get_directory(const fs::path& path)
    {
        if (path.has_extension())
        {
            // Concatenate with empty dir to ensure path ends with separator
            return path.parent_path() / "";
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
}
