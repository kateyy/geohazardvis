/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

bool checkEqual(const TextFileReader::FloatVectors & lhs, const TextFileReader::FloatVectors & rhs)
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

class TestQtTextFileReader : public TextFileReader
{
public:
    TestQtTextFileReader(const QString & fileName)
        : TextFileReader(TextFileReader::ImplementationID::Qt, fileName)
    {
    }
};

}


class TextFileReader_test : public ::testing::Test
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
    static const TextFileReader::FloatVectors & defaultContent()
    {
        static const TextFileReader::FloatVectors content = {
            { -1.0, std::numeric_limits<float>::quiet_NaN() }, // column 0
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

    TextFileReader::FloatVectors data;
    auto reader = TestQtTextFileReader(testFileName());
    const auto result = reader.read(data);

    ASSERT_TRUE(result.testFlag(TextFileReader::successful));
    ASSERT_TRUE(result.testFlag(TextFileReader::eof));

    ASSERT_TRUE(checkEqual(defaultContent(), data));
}

TEST_F(TextFileReader_test, LimitedLineCount_QByteArray)
{
    createTestFile();

    TextFileReader::FloatVectors data;
    auto reader = TestQtTextFileReader(testFileName());
    const auto result = reader.read(data, 1);

    ASSERT_TRUE(result.testFlag(TextFileReader::successful));
    ASSERT_FALSE(result.testFlag(TextFileReader::eof));
    ASSERT_EQ(16u, reader.filePos());

    auto firstRow = defaultContent();
    firstRow[0].resize(1);
    firstRow[1].resize(1);

    ASSERT_TRUE(checkEqual(firstRow, data));
}

TEST_F(TextFileReader_test, StartingFromOffset_QByteArray)
{
    createTestFile();

    TextFileReader::FloatVectors data;

    auto reader1 = TestQtTextFileReader(testFileName());
    const auto result1 = reader1.read(data, 1);
    ASSERT_TRUE(result1.testFlag(TextFileReader::successful));
    const auto filePos1 = reader1.filePos();
    reader1 = TestQtTextFileReader("");
    data.clear();

    auto reader2 = TestQtTextFileReader(testFileName());
    reader2.seekTo(filePos1);
    const auto result2 = reader2.read(data);
    ASSERT_TRUE(result2.testFlag(TextFileReader::successful));
    ASSERT_TRUE(result2.testFlag(TextFileReader::eof));

    TextFileReader::FloatVectors secondRow(2);
    secondRow[0].push_back(defaultContent()[0][1]);
    secondRow[1].push_back(defaultContent()[1][1]);

    ASSERT_TRUE(checkEqual(secondRow, data));
}

TEST_F(TextFileReader_test, ReportEOF_QByteArray)
{
    createTestFile();

    TextFileReader::FloatVectors data;

    const auto result = TestQtTextFileReader(testFileName()).read(data, 3);
    ASSERT_FALSE(result.testFlag(TextFileReader::successful));
    ASSERT_TRUE(result.testFlag(TextFileReader::eof));

    ASSERT_TRUE(checkEqual(defaultContent(), data));
}

TEST_F(TextFileReader_test, ReportInvalidFile_QByteArray)
{
    TextFileReader::FloatVectors data;

    const auto result = TestQtTextFileReader(testFileName()).read(data, 3);
    ASSERT_FALSE(result.testFlag(TextFileReader::successful));
    ASSERT_TRUE(result.testFlag(TextFileReader::invalidFile));
}
