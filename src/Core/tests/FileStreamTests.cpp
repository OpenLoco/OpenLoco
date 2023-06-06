#include <OpenLoco/Core/FileStream.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <numeric>

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

using DataBuffer = std::vector<uint8_t>;

static auto generateData(size_t dataLength)
{
    DataBuffer data(dataLength);
    for (size_t i = 0; i < dataLength; i++)
    {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    return data;
}

static void generateFile(const std::filesystem::path& filePath, const DataBuffer& inputBytes)
{
    FileStream streamOut(filePath, StreamMode::write);
    ASSERT_EQ(streamOut.getLength(), 0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    streamOut.write(inputBytes.data(), inputBytes.size());

    ASSERT_EQ(streamOut.getLength(), inputBytes.size());
    ASSERT_EQ(streamOut.getPosition(), inputBytes.size());
}

TEST(FileStreamTest, testWriteRead)
{
    const auto testDataSize = 4;
    const auto testData = generateData(testDataSize);
    const auto filePath = getTempFilePath();

    generateFile(filePath, testData);

    FileStream streamIn(filePath, StreamMode::read);
    ASSERT_EQ(streamIn.getLength(), testData.size());
    ASSERT_EQ(streamIn.getPosition(), 0);

    auto readBuffer = DataBuffer(testDataSize);
    streamIn.read(readBuffer.data(), readBuffer.size());

    ASSERT_EQ(readBuffer, testData);

    streamIn.close();
    std::filesystem::remove(filePath);
}

TEST(FileStreamTest, testBadRead)
{
    const auto testDataSize = 4;
    const auto testData = generateData(testDataSize);
    const auto filePath = getTempFilePath();

    generateFile(filePath, testData);

    FileStream streamIn(filePath, StreamMode::read);
    ASSERT_EQ(streamIn.getLength(), testData.size());
    ASSERT_EQ(streamIn.getPosition(), 0);

    streamIn.setPosition(2);
    ASSERT_EQ(streamIn.getPosition(), 2);

    std::array<uint8_t, 4> readBuffer{};
    EXPECT_THROW(streamIn.read(readBuffer.data(), readBuffer.size()), std::runtime_error);

    streamIn.close();
    std::filesystem::remove(filePath);
}

TEST(FileStreamTest, testPosition)
{
    const auto testDataSize = 4;
    const auto testData = generateData(testDataSize);
    const auto filePath = getTempFilePath();

    FileStream streamOut(filePath, StreamMode::write);
    ASSERT_EQ(streamOut.getLength(), 0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    streamOut.write(testData.data(), testData.size());

    ASSERT_EQ(streamOut.getLength(), testData.size());
    ASSERT_EQ(streamOut.getPosition(), testData.size());

    streamOut.setPosition(0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    streamOut.setPosition(100);
    ASSERT_EQ(streamOut.getPosition(), 100);

    streamOut.close();
    std::filesystem::remove(filePath);
}

TEST(FileStreamTest, testModeWriteRead)
{
    const auto testDataSize = 4;
    const auto testData = generateData(testDataSize);
    const auto filePath = getTempFilePath();

    FileStream streamOut(filePath, StreamMode::write);
    ASSERT_EQ(streamOut.getLength(), 0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    streamOut.write(testData.data(), testData.size());

    ASSERT_EQ(streamOut.getLength(), testData.size());
    ASSERT_EQ(streamOut.getPosition(), testData.size());

    streamOut.setPosition(0);
    ASSERT_EQ(streamOut.getPosition(), 0);

    DataBuffer readBuffer(testDataSize);
    EXPECT_THROW(streamOut.read(readBuffer.data(), readBuffer.size()), std::runtime_error);

    streamOut.close();
    std::filesystem::remove(filePath);
}

TEST(FileStreamTest, testModeReadWrite)
{
    const auto testDataSize = 4;
    const auto testData = generateData(testDataSize);
    const auto filePath = getTempFilePath();

    generateFile(filePath, testData);

    FileStream streamIn(filePath, StreamMode::read);
    ASSERT_EQ(streamIn.getLength(), testData.size());
    ASSERT_EQ(streamIn.getPosition(), 0);

    EXPECT_THROW(streamIn.write(testData.data(), testData.size()), std::runtime_error);

    streamIn.close();
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
