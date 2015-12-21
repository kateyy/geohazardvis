#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>

#include <core/io/BinaryFile.h>

#include <QFile>


class RawFile_test : public testing::Test
{
public:
    void SetUp() override
    {
        QFile::remove(testFileName);
    }
    void TearDown() override
    {
        QFile::remove(testFileName);
    }

    const char * testFileName = "BinaryFileTestFile.bin";
};

TEST_F(RawFile_test, Read)
{
    const std::vector<char> data{ 1, 2, 3, 4, 5 };

    std::ofstream refFile(testFileName);
    refFile.write(data.data(), data.size() * sizeof(decltype(data)::value_type));
    refFile.close();

    BinaryFile readFile(testFileName, BinaryFile::Read);
    ASSERT_TRUE(readFile.isReadable());
    ASSERT_FALSE(readFile.isWritable());
    ASSERT_EQ(readFile.pos(), 0u);

    std::remove_const<decltype(data)>::type readData;
    ASSERT_TRUE(readFile.read(data.size(), readData));
    ASSERT_EQ(data, readData);
    ASSERT_EQ(readFile.pos(), data.size() * sizeof(decltype(data)::value_type));
}

TEST_F(RawFile_test, FileSize)
{
    const std::vector<int> data{ 1, 2, 3, 4, 5, 6 };
    const size_t dataSize = data.size() * sizeof(decltype(data)::value_type);

    {
        std::ofstream out(testFileName);
        out.write(reinterpret_cast<const char *>(data.data()), dataSize);
    }

    BinaryFile file(testFileName, BinaryFile::Read);
    ASSERT_EQ(file.size(), dataSize);
}


TEST_F(RawFile_test, Write)
{
    const std::vector<char> data{ 1, 2, 3, 4, 5 };

    {
        BinaryFile writeFile(testFileName, BinaryFile::Write);
        ASSERT_FALSE(writeFile.isReadable());
        ASSERT_TRUE(writeFile.isWritable());
        ASSERT_EQ(writeFile.pos(), 0u);

        writeFile.write(data);
    }

    std::remove_const<decltype(data)>::type readData(data.size());
    std::ifstream refFile(testFileName);
    refFile.read(readData.data(), readData.size());

    ASSERT_EQ(refFile.gcount(), data.size());
    ASSERT_EQ(data, readData);
}

TEST_F(RawFile_test, WriteReadMultipleData)
{
    const std::vector<uint8_t> data1{ 1, 2, 3, 4, 5 };
    const std::vector<float> data2{ 1.f, 5.f };
    const std::vector<uint64_t> data3{ 4, 5, 8, 9, 10 };

    {
        BinaryFile out(testFileName, BinaryFile::Write);
        ASSERT_TRUE(out.write(data1));
        ASSERT_TRUE(out.write(data2));
        ASSERT_TRUE(out.write(data3));
    }

    BinaryFile in(testFileName, BinaryFile::Read);

    std::remove_const<decltype(data1)>::type readData1;
    std::remove_const<decltype(data2)>::type readData2;
    std::remove_const<decltype(data3)>::type readData3;

    ASSERT_TRUE(in.read(data1.size(), readData1));
    ASSERT_TRUE(in.read(data2.size(), readData2));
    ASSERT_TRUE(in.read(data3.size(), readData3));

    ASSERT_EQ(data1, readData1);
    ASSERT_EQ(data2, readData2);
    ASSERT_EQ(data3, readData3);

    ASSERT_EQ(in.size(),
        data1.size() * sizeof(decltype(data1)::value_type)
        + data2.size() * sizeof(decltype(data2)::value_type)
        + data3.size() * sizeof(decltype(data3)::value_type));
}

TEST_F(RawFile_test, Truncate)
{
    const std::vector<char> data{ 1, 2, 3, 4, 5 };

    std::ofstream refFile(testFileName);
    refFile.write(data.data(), data.size() * sizeof(decltype(data)::value_type));
    refFile.close();

    {
        BinaryFile rawFile(testFileName, BinaryFile::Write | BinaryFile::Truncate);
    }

    std::ifstream refReadFile(testFileName);
    char readData;
    refReadFile.read(&readData, 1u);

    ASSERT_EQ(refReadFile.gcount(), 0u);
}

TEST_F(RawFile_test, ReadFail)
{
    BinaryFile noIn(testFileName, BinaryFile::Read);

    ASSERT_FALSE(noIn.isReadable());
    ASSERT_FALSE(noIn.isWritable());
}

TEST_F(RawFile_test, SeekWrite)
{
    const std::vector<uint8_t> mainData{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
    const std::vector<float> midData{ 42.0f, 32.0f };

    const size_t mainDataSize = mainData.size() * sizeof(decltype(mainData)::value_type);
    const size_t midDataValueSize = sizeof(decltype(midData)::value_type);
    const size_t midDataSize = midData.size() * midDataValueSize;

    const size_t midBegin = 3u;
    const size_t midEnd = midBegin + midDataSize;

    {
        BinaryFile out(testFileName, BinaryFile::Write);
        out.write(mainData);
        out.seek(midBegin);
        ASSERT_TRUE(out.write(midData));
    }

    std::vector<uint8_t> readData;
    {
        BinaryFile in(testFileName, BinaryFile::Read);
        ASSERT_TRUE(in.read(mainDataSize, readData));
        ASSERT_EQ(readData.size(), mainData.size());
    }

    for (size_t i = 0; i < midBegin; ++i)
    {
        ASSERT_EQ(mainData[i], readData[i]);
    }
    for (size_t i = midBegin; i < midEnd; i += midDataValueSize)
    {
        const auto val = *reinterpret_cast<decltype(midData)::value_type *>(&readData.data()[i]);
        ASSERT_EQ(midData[i / midDataValueSize], val);
    }
    for (size_t i = midEnd; i < mainDataSize; ++i)
    {
        ASSERT_EQ(mainData[i], readData[i]);
    }
}
