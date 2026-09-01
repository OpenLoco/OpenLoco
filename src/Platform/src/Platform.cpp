#include "Platform.h"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <thread>

namespace OpenLoco::Platform
{
    fs::path showFolderPicker(const std::string& title)
    {
        using namespace std::literals;
        enum
        {
            failed,
            cancelled,
            inprogress,
            ok
        };
        struct userdata_t
        {
            fs::path dir;
            int status = inprogress;
        } data;

        auto callback = [](void* userdata, const char* const* filelist, int) {
            auto d = static_cast<userdata_t*>(userdata);

            if (!filelist)
            {
                d->status = failed;
                return;
            }
            if (!filelist[0])
            {
                d->status = cancelled;
                return;
            }

            d->dir = filelist[0];
            d->status = ok;
        };

        auto props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_FILE_DIALOG_TITLE_STRING, title.c_str());
        SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, false);

        SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFOLDER, callback, &data, props);

        // SDL's File dialogs are async, wait for it to close
        do
        {
            std::this_thread::sleep_for(100ms);
            SDL_PumpEvents();
        } while (data.status == inprogress);

        if (data.status != ok)
        {
            fprintf(stderr, "Platform::%s failed: ", __func__);
        }

        switch (data.status)
        {
            case ok:
                break;
            case failed:
                fprintf(stderr, "%s\n", SDL_GetError());
                break;
            default:
            case cancelled:
                fprintf(stderr, "%s\n", "User didn't pick a folder");
                break;
        }

        SDL_DestroyProperties(props);

        return data.dir;
    }
}
