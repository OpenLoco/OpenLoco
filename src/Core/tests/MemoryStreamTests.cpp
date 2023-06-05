#include <OpenLoco/Core/MemoryStream.h>
#include <array>
#include <gtest/gtest.h>

using namespace OpenLoco;

TEST(MemoryStreamTest, testWriteRead)
{
    MemoryStream ms;

    const std::array<uint8_t, 4> writeBuffer{ 0x01, 0x02, 0x03, 0x04 };
    ms.write(writeBuffer.data(), writeBuffer.size());

    ASSERT_EQ(ms.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(ms.getPosition(), sizeof(writeBuffer));

    ms.setPosition(0);
    ASSERT_EQ(ms.getPosition(), 0);

    std::array<uint8_t, 4> readBuffer{};
    ms.read(readBuffer.data(), readBuffer.size());

    ASSERT_EQ(readBuffer, writeBuffer);
}

TEST(MemoryStreamTest, testOverwrite)
{
    MemoryStream ms;

    const std::array<uint8_t, 4> writeBuffer{ 0x01, 0x02, 0x03, 0x04 };
    ms.write(writeBuffer.data(), writeBuffer.size());

    ASSERT_EQ(ms.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(ms.getPosition(), sizeof(writeBuffer));

    ms.setPosition(0);
    ASSERT_EQ(ms.getPosition(), 0);

    const std::array<uint8_t, 4> overwriteBuffer{ 0xCC, 0xCC, 0xCC, 0xCC };
    ms.write(overwriteBuffer.data(), overwriteBuffer.size());

    ASSERT_EQ(ms.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(ms.getPosition(), sizeof(writeBuffer));

    ms.setPosition(0);
    ASSERT_EQ(ms.getPosition(), 0);

    std::array<uint8_t, 4> readBuffer{};
    ms.read(readBuffer.data(), readBuffer.size());

    ASSERT_EQ(readBuffer, overwriteBuffer);
}

TEST(MemoryStreamTest, testPosition)
{
    MemoryStream ms;

    const std::array<uint8_t, 4> writeBuffer{ 0x01, 0x02, 0x03, 0x04 };
    ms.write(writeBuffer.data(), writeBuffer.size());

    ASSERT_EQ(ms.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(ms.getPosition(), sizeof(writeBuffer));

    ms.setPosition(0);
    ASSERT_EQ(ms.getPosition(), 0);

    ms.setPosition(100);
    ASSERT_EQ(ms.getPosition(), ms.getLength());
}
