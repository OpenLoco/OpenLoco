#include <OpenLoco/Core/Exception.hpp>
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

TEST(MemoryStreamTest, testBadRead)
{
    MemoryStream ms;

    const std::array<uint8_t, 4> writeBuffer{ 0x01, 0x02, 0x03, 0x04 };
    ms.write(writeBuffer.data(), writeBuffer.size());

    ASSERT_EQ(ms.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(ms.getPosition(), sizeof(writeBuffer));

    ms.setPosition(2);
    ASSERT_EQ(ms.getPosition(), 2);

    std::array<uint8_t, 4> readBuffer{};
    EXPECT_THROW(ms.read(readBuffer.data(), readBuffer.size()), Exception::RuntimeError);
}

TEST(MemoryStreamTest, testWrite100MiB)
{
    MemoryStream ms;

    const auto k100MiB = (1024U * 1024U) * 100U;
    const std::array<uint8_t, 1> writeBuffer{ 0xFF };

    for (size_t i = 0; i < k100MiB; i++)
    {
        ms.write(writeBuffer.data(), writeBuffer.size());
    }

    ASSERT_EQ(ms.getLength(), k100MiB);
}

TEST(MemoryStreamTest, testResize)
{
    MemoryStream ms;

    const auto kResizeSize = 100;

    ms.resize(kResizeSize);

    ASSERT_EQ(ms.getLength(), 100);
    ASSERT_EQ(ms.getPosition(), 0);

    // Write to the buffer to ensure it's writable
    std::byte* data = ms.data();
    for (size_t i = 0; i < kResizeSize; i++)
    {
        data[i] = std::byte{ 0xCC };
    }
}
