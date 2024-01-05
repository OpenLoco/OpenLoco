#include "Ui/TextInput.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include <OpenLoco/Interop/Interop.hpp>

#include <SDL2/SDL.h>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::TextInput
{
    // Common code from 0x0044685C, 0x004CE910
    bool InputSession::handleInput(uint32_t charCode, uint32_t keyCode)
    {
        if ((charCode >= SDLK_SPACE && charCode < SDLK_DELETE) || (charCode >= 159 && charCode <= 255))
        {
            if (inputLenLimit > 0 && buffer.length() == inputLenLimit)
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
        else if (charCode == SDLK_BACKSPACE)
        {
            if (cursorPosition == 0)
            {
                // Cursor is at beginning. No change required, but consume input
                return true;
            }

            buffer.erase(cursorPosition - 1, 1);
            cursorPosition -= 1;
        }
        else if (keyCode == SDLK_DELETE)
        {
            if (cursorPosition == buffer.length())
            {
                // Cursor is at end. No change required, but consume input
                return true;
            }

            buffer.erase(cursorPosition, 1);
        }
        else if (keyCode == SDLK_HOME)
        {
            cursorPosition = 0;
        }
        else if (keyCode == SDLK_END)
        {
            cursorPosition = buffer.length();
        }
        else if (keyCode == SDLK_LEFT)
        {
            if (cursorPosition == 0)
            {
                // Cursor is at beginning. No change required, but consume input
                return true;
            }

            cursorPosition -= 1;
        }
        else if (keyCode == SDLK_RIGHT)
        {
            if (cursorPosition == buffer.length())
            {
                // Cursor is at end. No change required, but consume input
                return true;
            }

            cursorPosition += 1;
        }

        cursorFrame = 0;
        return true;
    }

    bool InputSession::needsReoffsetting(int16_t containerWidth)
    {
        std::string cursorStr = buffer.substr(0, cursorPosition);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        auto stringWidth = drawingCtx.getStringWidth(buffer.c_str());
        auto cursorX = drawingCtx.getStringWidth(cursorStr.c_str());

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

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        auto stringWidth = drawingCtx.getStringWidth(buffer.c_str());
        auto cursorX = drawingCtx.getStringWidth(cursorStr.c_str());

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

    void InputSession::clearInput()
    {
        buffer.clear();
        cursorPosition = 0;
        cursorFrame = 0;
        xOffset = 0;
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
