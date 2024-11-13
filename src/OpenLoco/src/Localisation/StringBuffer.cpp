#include "StringBuffer.h"
#include "Formatting.h"

namespace OpenLoco
{

    StringBuffer::StringBuffer(value_type* buffer, size_t maxLen)
        : buffer(buffer)
        , offset(0)
        , maxLen(maxLen)
    {
    }

    void StringBuffer::appendData(const void* data, size_t size)
    {
        if (offset + size >= maxLen)
        {
            throw Exception::OverflowError("String buffer overflow");
        }

        std::memcpy(buffer + offset, data, size);
        offset += size;
    }

    void StringBuffer::append(value_type chr)
    {
        if (offset >= maxLen)
        {
            throw Exception::OverflowError("String buffer overflow");
        }

        buffer[offset] = chr;
        offset++;
    }

    void StringBuffer::append(const char* input)
    {
        return append(input, 0xFFFFFFFFU);
    }

    void StringBuffer::append(const char* input, size_t inputLen)
    {
        for (size_t i = 0; i < inputLen;)
        {
            auto ch = input[i];
            if (ch == '\0')
            {
                break;
            }

            if (ch >= ControlCodes::oneArgBegin && ch < ControlCodes::oneArgEnd)
            {
                append(ch);
                i++;
            }
            else if (ch >= ControlCodes::twoArgBegin && ch < ControlCodes::twoArgEnd)
            {
                if (i + 2 > inputLen)
                {
                    throw Exception::OverflowError("String buffer overflow");
                }
                appendData(input + i, 2);
                i += 2;
            }
            else if (ch >= ControlCodes::fourArgBegin && ch < ControlCodes::fourArgEnd)
            {
                if (i + 4 > inputLen)
                {
                    throw Exception::OverflowError("String buffer overflow");
                }
                appendData(input + i, 4);
                i += 4;
            }
            else
            {
                append(ch);
                i++;
            }
        }
    }

    // std::back_inserter support.
    void StringBuffer::push_back(value_type chr)
    {
        append(chr);
    }

    char* StringBuffer::current() const
    {
        return buffer + offset;
    }

    void StringBuffer::nullTerminate()
    {
        if (offset < maxLen)
        {
            buffer[offset] = '\0';
        }
        else
        {
            buffer[maxLen - 1] = '\0';
        }
    }

    void StringBuffer::grow(size_t numChars)
    {
        if (offset + numChars >= maxLen)
        {
            throw std::overflow_error("String buffer overflow");
        }
        offset += numChars;
    }

}
