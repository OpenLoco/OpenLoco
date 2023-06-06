#include <OpenLoco/Core/FileStream.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>

using namespace OpenLoco;

static std::filesystem::path getTempFilePath()
{
    char tempNameBuf[L_tmpnam]{};
#ifdef _MSC_VER
    tmpnam_s(tempNameBuf, L_tmpnam);
    const char* tempName = tempNameBuf;
#else
    const char* tempName = tmpnam(tempNameBuf);
#endif
    auto tempDir = std::filesystem::temp_directory_path();
    auto tempFile = tempDir / tempName;
    return tempFile;
}

TEST(FileStreamTest, testWriteRead)
{
    const auto filePath = getTempFilePath();

    FileStream streamOut(filePath, StreamMode::write);
    ASSERT_EQ(streamOut.getLength(), 0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    const std::array<uint8_t, 4> writeBuffer{ 0x01, 0x02, 0x03, 0x04 };
    streamOut.write(writeBuffer.data(), writeBuffer.size());

    ASSERT_EQ(streamOut.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(streamOut.getPosition(), sizeof(writeBuffer));

    streamOut.close();

    FileStream streamIn(filePath, StreamMode::read);
    ASSERT_EQ(streamIn.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(streamIn.getPosition(), 0);

    std::array<uint8_t, 4> readBuffer{};
    streamIn.read(readBuffer.data(), readBuffer.size());

    ASSERT_EQ(readBuffer, writeBuffer);

    streamIn.close();
    std::filesystem::remove(filePath);
}

TEST(FileStreamTest, testPosition)
{
    const auto filePath = getTempFilePath();

    FileStream streamOut(filePath, StreamMode::write);
    ASSERT_EQ(streamOut.getLength(), 0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    const std::array<uint8_t, 4> writeBuffer{ 0x01, 0x02, 0x03, 0x04 };
    streamOut.write(writeBuffer.data(), writeBuffer.size());

    ASSERT_EQ(streamOut.getLength(), sizeof(writeBuffer));
    ASSERT_EQ(streamOut.getPosition(), sizeof(writeBuffer));

    streamOut.setPosition(0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    streamOut.setPosition(100);
    ASSERT_EQ(streamOut.getPosition(), 100);

    streamOut.close();
    std::filesystem::remove(filePath);
}

TEST(FileStreamTest, testWrite100MiB)
{
    const auto filePath = getTempFilePath();

    FileStream streamOut(filePath, StreamMode::write);
    ASSERT_EQ(streamOut.getLength(), 0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    const auto k100MiB = (1024U * 1024U) * 100U;
    const std::array<uint8_t, 1> writeBuffer{ 0xFF };

    for (size_t i = 0; i < k100MiB; i++)
    {
        streamOut.write(writeBuffer.data(), writeBuffer.size());
    }

    ASSERT_EQ(streamOut.getLength(), k100MiB);

    streamOut.close();
    std::filesystem::remove(filePath);
}
