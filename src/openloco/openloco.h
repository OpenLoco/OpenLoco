#pragma once

#include "utility/prng.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace openloco::map
{
    class tilemanager;
}

namespace openloco::ui
{
    class viewportmanager;
}

namespace openloco
{
    class companymanager;
    class industrymanager;
    class messagemanager;
    class objectmanager;
    class scenariomanager;
    class stationmanager;
    class thingmanager;
    class townmanager;

    class context
    {
    private:
        std::unique_ptr<companymanager> _companymgr;
        std::unique_ptr<industrymanager> _industrymgr;
        std::unique_ptr<objectmanager> _objectmgr;
        std::unique_ptr<map::tilemanager> _tilemgr;
        std::unique_ptr<messagemanager> _messagemgr;
        std::unique_ptr<scenariomanager> _scenariomgr;
        std::unique_ptr<stationmanager> _stationmgr;
        std::unique_ptr<thingmanager> _thingmgr;
        std::unique_ptr<townmanager> _townmgr;
        std::unique_ptr<ui::viewportmanager> _viewportmgr;

    public:
        context();
        context(const context&) = delete;
        ~context();

        template<typename T>
        T& get();

        template<typename T>
        const T& get() const { return ((context*)this)->get<T>(); }

        template<>
        companymanager& get() { return *_companymgr; }
        template<>
        industrymanager& get() { return *_industrymgr; }
        template<>
        messagemanager& get() { return *_messagemgr; }
        template<>
        objectmanager& get() { return *_objectmgr; }
        template<>
        map::tilemanager& get() { return *_tilemgr; }
        template<>
        scenariomanager& get() { return *_scenariomgr; }
        template<>
        stationmanager& get() { return *_stationmgr; }
        template<>
        thingmanager& get() { return *_thingmgr; }
        template<>
        townmanager& get() { return *_townmgr; }
        template<>
        ui::viewportmanager& get() { return *_viewportmgr; }
    };

    extern context g_ctx;

    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t unknown_2 = 1 << 2;
        constexpr uint8_t unknown_3 = 1 << 3;
        constexpr uint8_t unknown_4 = 1 << 4;
        constexpr uint8_t unknown_5 = 1 << 5;
        constexpr uint8_t unknown_6 = 1 << 6;
    }

    extern const char version[];

    std::string get_version_info();

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    uint8_t get_screen_flags();
    bool is_editor_mode();
    bool is_title_mode();
    bool is_unknown_4_mode();
    bool is_paused();
    uint32_t scenario_ticks();
    utility::prng& gprng();

    void main();
    void prompt_tick_loop(std::function<bool()> tickAction);
}
