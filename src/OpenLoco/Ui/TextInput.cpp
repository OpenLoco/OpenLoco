#include "../Ui/TextInput.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringManager.h"
#include "../Win32.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

// `interface` is defined as a macro for `struct` in `windows.h`
#undef interface
#endif

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::TextInput
{
    static loco_global<int32_t, 0x0112C876> _currentFontSpriteBase;

    // Common code from 0x0044685C, 0x004CE910
    bool InputSession::handleInput(uint32_t charCode, uint32_t keyCode)
    {
        if ((charCode >= VK_SPACE && charCode < VK_F12) || (charCode >= 159 && charCode <= 255))
        {
            if (buffer.length() == 199)
            {
                return false;
            }

            if (cursorPosition == buffer.length())
            {
                buffer.append(1, (char)charCode);
            }
            else
            {
                buffer.insert(cursorPosition, 1, (char)charCode);
            }
            cursorPosition += 1;
        }
        else if (charCode == VK_BACK)
        {
            if (cursorPosition == 0)
            {
                // Cursor is at beginning
                return false;
            }

            buffer.erase(cursorPosition - 1, 1);
            cursorPosition -= 1;
        }
        else if (keyCode == VK_DELETE)
        {
            if (cursorPosition == buffer.length())
            {
                return false;
            }

            buffer.erase(cursorPosition, 1);
        }
        else if (keyCode == VK_HOME)
        {
            cursorPosition = 0;
        }
        else if (keyCode == VK_END)
        {
            cursorPosition = buffer.length();
        }
        else if (keyCode == VK_LEFT)
        {
            if (cursorPosition == 0)
            {
                // Cursor is at beginning
                return false;
            }

            cursorPosition -= 1;
        }
        else if (keyCode == VK_RIGHT)
        {
            if (cursorPosition == buffer.length())
            {
                // Cursor is at end
                return false;
            }

            cursorPosition += 1;
        }

        cursorFrame = 0;
        return true;
    }

    bool InputSession::needsReoffsetting(int16_t containerWidth)
    {
        std::string cursorStr = buffer.substr(0, cursorPosition);

        _currentFontSpriteBase = Font::medium_bold;
        auto stringWidth = Gfx::getStringWidth(buffer.c_str());
        auto cursorX = Gfx::getStringWidth(cursorStr.c_str());

        int x = xOffset + cursorX;

        if (x < textboxPadding)
            return true;

        if (x > containerWidth - textboxPadding)
            return true;

        if (xOffset + stringWidth < containerWidth - textboxPadding)
            return true;

        return false;
    }

    /**
     * 0x004CEB67
     *
     * @param containerWidth @<edx>
     */
    void InputSession::calculateTextOffset(int16_t containerWidth)
    {
        std::string cursorStr = buffer.substr(0, cursorPosition);

        _currentFontSpriteBase = Font::medium_bold;
        auto stringWidth = Gfx::getStringWidth(buffer.c_str());
        auto cursorX = Gfx::getStringWidth(cursorStr.c_str());

        auto midX = containerWidth / 2;

        // Prefer to centre cursor
        xOffset = -1 * (cursorX - midX);

        // Make sure that text will always be at the left edge
        int16_t minOffset = textboxPadding;
        int16_t maxOffset = textboxPadding;

        if (stringWidth + textboxPadding * 2 > containerWidth)
        {
            // Make sure that the whole textbox is filled up
            minOffset = -stringWidth + containerWidth - textboxPadding;
        }
        xOffset = std::clamp<int16_t>(xOffset, minOffset, maxOffset);
    }

    // 0x004CEBFB
    void InputSession::sanitizeInput()
    {
        buffer.erase(
            std::remove_if(
                buffer.begin(),
                buffer.end(),
                [](unsigned char chr) {
                    if (chr < ' ')
                    {
                        return true;
                    }
                    else if (chr <= 'z')
                    {
                        return false;
                    }
                    else if (chr == 171)
                    {
                        return false;
                    }
                    else if (chr == 187)
                    {
                        return false;
                    }
                    else if (chr >= 191)
                    {
                        return false;
                    }

                    return true;
                }),
            buffer.end());
    }
}
