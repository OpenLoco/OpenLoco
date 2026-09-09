#include "Ui/TextInput.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Conversion.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"

#include <SDL3/SDL_keycode.h>

namespace OpenLoco::Ui::TextInput
{
    static constexpr std::size_t kUnicodeEscapeSize = 5;

    static bool isUnicodeEscapeAt(const std::string& buffer, std::size_t pos)
    {
        return pos + kUnicodeEscapeSize <= buffer.size()
            && static_cast<uint8_t>(buffer[pos]) == ControlCodes::unicode;
    }

    static std::size_t glyphSizeAt(const std::string& buffer, std::size_t pos)
    {
        return isUnicodeEscapeAt(buffer, pos) ? kUnicodeEscapeSize : 1;
    }

    static std::size_t glyphSizeBefore(const std::string& buffer, std::size_t pos)
    {
        if (pos >= kUnicodeEscapeSize && isUnicodeEscapeAt(buffer, pos - kUnicodeEscapeSize))
        {
            return kUnicodeEscapeSize;
        }
        return pos > 0 ? 1 : 0;
    }

    // Common code from 0x0044685C, 0x004CE910
    bool InputSession::handleInput(uint32_t charCode, uint32_t keyCode)
    {
        if ((charCode >= SDLK_SPACE && charCode != SDLK_DELETE) || (charCode >= 159 && charCode <= 255))
        {
            char encoded[5];
            const auto n = Localisation::writeLocoChar(encoded, charCode);
            if (inputLenLimit > 0 && buffer.length() + n > inputLenLimit)
            {
                // Limit reached but we need to consume this input.
                return true;
            }

            buffer.insert(cursorPosition, encoded, n);
            cursorPosition += n;
        }
        else if (charCode == SDLK_BACKSPACE)
        {
            if (cursorPosition == 0)
            {
                // Cursor is at beginning. No change required, but consume input
                return true;
            }

            const auto n = glyphSizeBefore(buffer, cursorPosition);
            cursorPosition -= n;
            buffer.erase(cursorPosition, n);
        }
        else if (keyCode == SDLK_DELETE)
        {
            if (cursorPosition == buffer.length())
            {
                // Cursor is at end. No change required, but consume input
                return true;
            }

            buffer.erase(cursorPosition, glyphSizeAt(buffer, cursorPosition));
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

            cursorPosition -= glyphSizeBefore(buffer, cursorPosition);
        }
        else if (keyCode == SDLK_RIGHT)
        {
            if (cursorPosition == buffer.length())
            {
                // Cursor is at end. No change required, but consume input
                return true;
            }

            cursorPosition += glyphSizeAt(buffer, cursorPosition);
        }

        cursorFrame = 0;
        return true;
    }

    bool InputSession::needsReoffsetting(int16_t containerWidth)
    {
        std::string cursorStr = buffer.substr(0, cursorPosition);

        const auto font = Gfx::Font::medium_bold;
        const auto stringWidth = Gfx::TextRenderer::getStringWidth(font, buffer.c_str());
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
        std::string cursorStr = buffer.substr(0, cursorPosition);

        const auto font = Gfx::Font::medium_bold;
        const auto stringWidth = Gfx::TextRenderer::getStringWidth(font, buffer.c_str());
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

    // 0x004CEBFB
    void InputSession::sanitizeInput()
    {
        std::string out;
        out.reserve(buffer.size());
        for (std::size_t i = 0; i < buffer.size();)
        {
            if (isUnicodeEscapeAt(buffer, i))
            {
                out.append(buffer, i, kUnicodeEscapeSize);
                i += kUnicodeEscapeSize;
                continue;
            }

            const auto chr = static_cast<unsigned char>(buffer[i]);
            const bool keep = (chr >= ' ' && chr <= 'z') || chr == 171 || chr == 187 || chr >= 191;
            if (keep)
            {
                out.push_back(buffer[i]);
            }
            ++i;
        }
        buffer = std::move(out);
        if (cursorPosition > buffer.size())
        {
            cursorPosition = buffer.size();
        }
    }
}
