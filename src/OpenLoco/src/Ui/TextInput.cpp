#include "Ui/TextInput.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "Localisation/Unicode.h"

#include <SDL3/SDL_keycode.h>

namespace OpenLoco::Ui::TextInput
{
    // Common code from 0x0044685C, 0x004CE910
    bool InputSession::handleInput(uint32_t charCode, uint32_t keyCode)
    {
        if ((charCode >= SDLK_SPACE && charCode < SDLK_DELETE) || (charCode >= 159))
        {
            if (inputLenLimit > 0 && length() >= inputLenLimit)
            {
                // Limit reached but we need to consume this input.
                return true;
            }

            auto character = Localisation::codepointToUtf8(charCode);

            if (cursorPosition == length())
            {
                buffer.append(character);
            }
            else
            {
                Localisation::utf8Insert(buffer, cursorPosition, character);
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

            Localisation::utf8Delete(buffer, cursorPosition - 1);
            cursorPosition -= 1;
        }
        else if (keyCode == SDLK_DELETE)
        {
            if (cursorPosition == length())
            {
                // Cursor is at end. No change required, but consume input
                return true;
            }

            Localisation::utf8Delete(buffer, cursorPosition);
        }
        else if (keyCode == SDLK_HOME)
        {
            cursorPosition = 0;
        }
        else if (keyCode == SDLK_END)
        {
            cursorPosition = length();
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
            if (cursorPosition == length())
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
        const auto locoString = loco();
        std::string cursorStr = locoString.substr(0, cursorPosition);

        const auto font = Gfx::Font::medium_bold;
        const auto stringWidth = Gfx::TextRenderer::getStringWidth(font, locoString.c_str());
        const auto cursorX = Gfx::TextRenderer::getStringWidth(font, cursorStr.c_str());

        const int x = xOffset + cursorX;

        if (x < textboxPadding)
        {
            return true;
        }

        if (x > containerWidth - textboxPadding)
        {
            return true;
        }

        if (xOffset + stringWidth < containerWidth - textboxPadding)
        {
            return true;
        }

        return false;
    }

    /**
     * 0x004CEB67
     *
     * @param containerWidth @<edx>
     */
    void InputSession::calculateTextOffset(int16_t containerWidth)
    {
        const auto locoString = loco();
        std::string cursorStr = locoString.substr(0, cursorPosition);

        const auto font = Gfx::Font::medium_bold;
        const auto stringWidth = Gfx::TextRenderer::getStringWidth(font, locoString.c_str());
        const auto cursorX = Gfx::TextRenderer::getStringWidth(font, cursorStr.c_str());

        const auto midX = containerWidth / 2;

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

    std::string InputSession::loco() const
    {
        return Localisation::convertUnicodeToLoco(buffer);
    }

    std::string InputSession::utf8() const
    {
        return buffer;
    }

    size_t InputSession::length() const
    {
        return Localisation::utf8Length(buffer);
    }
}
