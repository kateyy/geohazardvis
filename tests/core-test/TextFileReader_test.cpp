#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <core/io/TextFileReader.h>

#include "TestEnvironment.h"


namespace
{

bool checkEqual(const io::InputVector & lhs, const io::InputVector & rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (size_t c = 0; c < lhs.size(); ++c)
    {
        auto l_col = lhs[c];
        auto r_col = rhs[c];
        if (l_col.size() != r_col.size())
        {
            return false;
        }

        for (size_t r = 0; r < l_col.size(); ++r)
        {
            if (l_col[r] != r_col[r]
                && !(std::isnan(l_col[r]) && std::isnan(r_col[r])))
            {
                return false;
            }
        }
    }

    return true;
}

}


class TextFileReader_test : public testing::Test
{
public:
    static const QString & testFileName()
    {
        static QString fileName = QDir(TestEnvironment::testDirPath()).filePath("TextFileParser_test_file.txt");
        return fileName;
    }

    static const QString & defaultContentString()
    {
        static const QString content = [] ()
        {
            QString c;
            QTextStream stream(&c);

            stream << "   -1.0    1.0\r\n";
            stream << "    NaN    2.0\n\n";
            return c;
        }();

        return content;
    }
    static const io::InputVector & defaultContent()
    {
        static const io::InputVector content = {
            { -1.0, std::numeric_limits<io::t_FP>::quiet_NaN() }, // column 0
            { 1.0, 2.0 } // column 1
        };

        return content;
    }

    void SetUp() override
    {
        TestEnvironment::createTestDir();
    }
    void TearDown() override
    {
        TestEnvironment::clearTestDir();
    }

    void createTestFile(const QString & content = defaultContentString())
    {
        QFile testFile(testFileName());
        testFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        QTextStream ostream(&testFile);
        ostream << content;
    }
};


TEST_F(TextFileReader_test, BasicReadTest_qFile_QByteArray)
{
    createTestFile();

    io::InputVector data;

    const auto result = TextFileReader::Impl::qFile_QByteArray(testFileName(), data, 0, 0);

    ASSERT_EQ(TextFileReader::Result::noError, result.state);
    ASSERT_EQ(defaultContentString().size(), result.filePos);

    ASSERT_TRUE(checkEqual(defaultContent(), data));
}

TEST_F(TextFileReader_test, LimitedValueCount_QByteArray)
{
    createTestFile();

    io::InputVector data;

    const auto result = TextFileReader::Impl::qFile_QByteArray(testFileName(), data, 0, 2);

    ASSERT_EQ(TextFileReader::Result::noError, result.state);
    //ASSERT_EQ(defaultContentString().size(), result.filePos);

    auto firstRow = defaultContent();
    firstRow[0].resize(1);
    firstRow[1].resize(1);

    ASSERT_TRUE(checkEqual(firstRow, data));
}

TEST_F(TextFileReader_test, StartingFromOffset_QByteArray)
{
    createTestFile();

    io::InputVector data;

    const auto result1 = TextFileReader::Impl::qFile_QByteArray(testFileName(), data, 0, 2);
    data.clear();
    const auto result2 = TextFileReader::Impl::qFile_QByteArray(testFileName(), data, result1.filePos, 0);
    ASSERT_EQ(TextFileReader::Result::noError, result2.state);

    io::InputVector secondRow(2);
    secondRow[0].push_back(defaultContent()[0][1]);
    secondRow[1].push_back(defaultContent()[1][1]);

    ASSERT_TRUE(checkEqual(secondRow, data));
}
