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

#include "TextFileWriter.h"

#include <algorithm>
#include <limits>
#include <type_traits>

#include <QDebug>
#include <QFile>

#include <vtkArrayDispatch.h>
#include <vtkDataArray.h>
#include <vtkDataArrayAccessor.h>
#include <vtkStringArray.h>
#include <vtkUnicodeStringArray.h>

#include <core/data_objects/DataObject.h>
#include <core/utility/qthelper.h>


namespace
{

QString platformLineEnding()
{
#if WIN32
    return "\r\n";
#else
    return "\n";
#endif
}

}

class ArraysToDelimitedText
{
public:
    QString m_delimiter;
    QString m_stringDelimiter;
    QString m_lineEnding;

public:
    ArraysToDelimitedText()
        : m_delimiter{ ";" }
        , m_stringDelimiter{ "\"" }
        , m_lineEnding{ platformLineEnding() }
    {
    }

    /** Write a vtkDataArray, vtkStringArray, vtkUnicodeStringArray, or return false. */
    bool writeArrays(
        QIODevice & output,
        const std::vector<vtkSmartPointer<vtkAbstractArray>> & arrays,
        const std::vector<vtkIdType> & indices)
    {
        assert(output.isOpen() && output.isWritable());

        if (std::find(arrays.begin(), arrays.end(), nullptr) != arrays.end())
        {
            qWarning() << "Passed null array to delimited text output. Aborting";
            return false;
        }

        const auto stringDelim = m_stringDelimiter.toUtf8();

        std::vector<std::unique_ptr<AbstractValueWriter>> writers;
        for (const auto & array : arrays)
        {
            if (array->GetNumberOfComponents() == 0)
            {
                continue;
            }

            if (auto dataArray = vtkDataArray::SafeDownCast(array))
            {
                writers.push_back(
                    std::make_unique<DataArrayValueWriter>(std::ref(output), *dataArray));
                continue;
            }
            if (auto stringArray = vtkStringArray::SafeDownCast(array))
            {
                writers.push_back(
                    std::make_unique<StringArrayValueWriter>(std::ref(output),
                        *stringArray, stringDelim));
                continue;
            }
            if (auto stringArray = vtkUnicodeStringArray::SafeDownCast(array))
            {
                writers.push_back(
                    std::make_unique<UnicodeStringArrayValueWriter>(std::ref(output),
                        *stringArray, stringDelim));
                continue;
            }
            qWarning() << "Cannot write array" << array->GetName() << ", type is unsupported:" 
                << array->GetDataTypeAsString();
            return false;
        }

        const auto delim = m_delimiter.toUtf8();
        const auto eol = m_lineEnding.toUtf8();

        auto printRow = [&output, &writers, delim, eol] (const vtkIdType row) -> bool
        {
            for (auto & writer : writers)
            {
                if (row >= writer->numTuples)
                {
                    qWarning() << "Tuple with index" << row << "is not available in array"
                        << writer->arrayName() << ". Aborting.";
                    return false;
                }
                for (int c = 0; c < writer->numComponents - 1; ++c)
                {
                    if (!writer->writeValue(row, c))
                    {
                        return false;
                    }
                    output.write(delim);
                }
                if (!writer->writeValue(row, writer->numComponents - 1))
                {
                    return false;
                }
                if (writer != writers.back())
                {
                    output.write(delim);
                }
            }
            output.write(eol);
            return true;
        };

        if (!indices.empty())
        {
            for (const vtkIdType idx : indices)
            {
                if (!printRow(idx))
                {
                    return false;
                }
            }
            return true;
        }

        const vtkIdType numTuples = arrays.front()->GetNumberOfTuples();
        for (auto & array : arrays)
        {
            if (array->GetNumberOfTuples() != numTuples)
            {
                qWarning() << "Delimited text output: Expecting all arrays to have the same "
                    << " number of tuples. Aborting.";
                return false;
            }
        }
        
        for (vtkIdType i = 0; i < numTuples; ++i)
        {
            if (!printRow(i))
            {
                return false;
            }
        }

        return true;
    }


private:
    /** Utility struct that dispatches between different types of string values. */
    struct AbstractValueWriter
    {
        AbstractValueWriter(QIODevice & output, const int numComponents, const vtkIdType numTuples)
            : numComponents{ numComponents }
            , numTuples{ numTuples }
            , output{ output }
        {
        }
        virtual ~AbstractValueWriter() = default;
        virtual bool writeValue(const vtkIdType tupleIndex, const int componentIndex) = 0;
        virtual const char * arrayName() = 0;
        const int numComponents;
        const vtkIdType numTuples;
    protected:
        QIODevice & output;
    };

    template<typename Array_t> struct ValueWriter : public AbstractValueWriter
    {
        ValueWriter(QIODevice & output, Array_t & array)
            : AbstractValueWriter(output,
                array.GetNumberOfComponents(), array.GetNumberOfTuples())
            , array{ array }
        {
        }
        const char * arrayName() override
        {
            return array.GetName();
        }
    protected:
        Array_t & array;
    };

    struct StringArrayValueWriter : public ValueWriter<vtkStringArray>
    {
        StringArrayValueWriter(QIODevice & output, vtkStringArray & array,
            const QByteArray & stringDelimiter)
            : ValueWriter<vtkStringArray>(output, array)
            , stringDelimiter{ stringDelimiter }
        {
        }
        bool writeValue(const vtkIdType tupleIndex, const int componentIndex) override
        {
            output.write(stringDelimiter);
            const auto & string = array.GetValue(tupleIndex * array.GetNumberOfComponents() + componentIndex);
            if (!string.empty())
            {
                auto size = static_cast<qint64>(string.size());
                if (string.size() > static_cast<size_t>(std::numeric_limits<qint64>::max()))
                {
                    qWarning() << "Delimited Text Output: string value is too long, clamping "
                        << "(row " << tupleIndex << ", component" << componentIndex << ")";
                    size = std::numeric_limits<qint64>::max();
                }
                if (output.write(string.data(), size) != size)
                {
                    qWarning() << "Delimited Text Output: Failure while writing string"
                        << "(row " << tupleIndex << ", component" << componentIndex << "). Aborting.";
                    return false;
                }
            }
            output.write(stringDelimiter);
            return true;
        }
    protected:
        const QByteArray stringDelimiter;
    };
    struct UnicodeStringArrayValueWriter : public ValueWriter<vtkUnicodeStringArray>
    {
        UnicodeStringArrayValueWriter(QIODevice & output, vtkUnicodeStringArray & array,
            const QByteArray & stringDelimiter)
            : ValueWriter<vtkUnicodeStringArray>(output, array)
            , stringDelimiter{ stringDelimiter }
        {
        }
        bool writeValue(const vtkIdType tupleIndex, const int componentIndex) override
        {
            output.write(stringDelimiter);
            const auto & string = array.GetValue(tupleIndex * array.GetNumberOfComponents() + componentIndex);
            if (!string.empty())
            {
                auto size = static_cast<qint64>(string.byte_count());
                if (string.byte_count() > static_cast<size_t>(std::numeric_limits<qint64>::max()))
                {
                    qWarning() << "Delimited Text Output: string value is too long, clamping "
                        << "(row " << tupleIndex << ", component" << componentIndex << ")";
                    size = std::numeric_limits<qint64>::max();
                }
                if (output.write(string.utf8_str(), size) != size)
                {
                    qWarning() << "Delimited Text Output: Failure while writing string "
                        << "(row " << tupleIndex << ", component" << componentIndex << "). Aborting.";
                    return false;
                }
            }
            output.write(stringDelimiter);
            return true;
        }
    protected:
        const QByteArray stringDelimiter;
    };
    struct DataArrayValueWriter : public ValueWriter<vtkDataArray>
    {
        DataArrayValueWriter(QIODevice & output, vtkDataArray & array)
            : ValueWriter<vtkDataArray>(output, array)
            , m_component{ 0 }
            , m_okay{ false }
        {
        }
        bool writeValue(const vtkIdType tupleIndex, const int componentIndex) override
        {
            m_tuple = tupleIndex;
            m_component = componentIndex;
            if (!vtkArrayDispatch::Dispatch::Execute(&array, *this))
            {
                this->operator()(&array);
            }
            return m_okay;
        }

        template<typename Array_t>
        void operator()(Array_t * array_t)
        {
            using Value_t = typename vtkDataArrayAccessor<Array_t>::APIType;
            const Value_t value = vtkDataArrayAccessor<Array_t>(array_t).Get(m_tuple, m_component);
            auto && data = QByteArray::number(static_cast<QtNumberType_t<Value_t>>(value));
            const int size = data.size();
            if (size != output.write(std::forward<QByteArray>(data)))
            {
                qWarning() << "Delimited Text Output: Failure while writing value "
                    << "(row " << m_tuple << ", component" << m_component << "). Aborting.";
                m_okay = false;
                return;
            }
            m_okay = true;
        }

    private:
        vtkIdType m_tuple;
        int m_component;
        bool m_okay;
    };
};


TextFileWriter::TextFileWriter()
    : m_writer{ std::make_unique<ArraysToDelimitedText>() }
{
}

TextFileWriter::TextFileWriter(std::nullptr_t)
    : m_writer{}
{
}

void TextFileWriter::resetOutput()
{
    m_outputFileName.clear();
    m_outputFile.reset();
    m_outputTarget.clear();
}

QIODevice * TextFileWriter::output()
{
    if (m_outputTarget)
    {
        return m_outputTarget;
    }

    return m_outputFile.get();
}

TextFileWriter::~TextFileWriter() = default;

TextFileWriter::TextFileWriter(TextFileWriter && other)
    : TextFileWriter(nullptr)
{
    swap(*this, other);
}

TextFileWriter & TextFileWriter::operator=(TextFileWriter && other)
{
    swap(*this, other);
    return *this;
}

bool TextFileWriter::setOutputFileName(const QString & fileName, const bool truncate)
{
    resetOutput();
    m_outputFileName = fileName;
    if (m_outputFileName.isEmpty())
    {
        return false;
    }

    auto file = std::make_unique<QFile>(m_outputFileName);
    if (!file->open(QIODevice::ReadWrite | (truncate ? QIODevice::Truncate : QIODevice::Append)))
    {
        return false;
    }
    m_outputFile = std::move(file);
    return true;
}

const QString & TextFileWriter::outputFileName() const
{
    return m_outputFileName;
}

bool TextFileWriter::setOutputTarget(QIODevice * qioDevice, const bool truncate)
{
    resetOutput();
    if (!qioDevice)
    {
        return false;
    }

    m_outputTarget = qioDevice;
    if (!m_outputTarget->isOpen() || !m_outputTarget->isWritable() || truncate)
    {
        if (!m_outputTarget->open(QIODevice::ReadWrite |
            (truncate ? QIODevice::Truncate : QIODevice::Append)))
        {
            return false;
        }
    }
    return true;
}

QIODevice * TextFileWriter::outputTarget() const
{
    return m_outputTarget;
}

const QString & TextFileWriter::delimiter() const
{
    return m_writer->m_delimiter;
}

void TextFileWriter::setDelimiter(const QString & delimiter)
{
    m_writer->m_delimiter = delimiter;
}

const QString & TextFileWriter::stringDelimiter() const
{
    return m_writer->m_stringDelimiter;
}

void TextFileWriter::setStringDelimiter(const QString & stringDelimiter)
{
    m_writer->m_stringDelimiter = stringDelimiter;
}

const QString & TextFileWriter::lineEnding() const
{
    return m_writer->m_lineEnding;
}

void TextFileWriter::setLineEnding(const QString & lineEnding)
{
    if (lineEnding.isEmpty())
    {
        m_writer->m_lineEnding = platformLineEnding();
    }
    else
    {
        m_writer->m_lineEnding = lineEnding;
    }
}

bool TextFileWriter::clearFile()
{
    auto outFile = dynamic_cast<QFile *>(output());
    if (!outFile)
    {
        return false;
    }

    outFile->close();
    return outFile->open(QIODevice::Truncate | QIODevice::ReadWrite);
}

bool TextFileWriter::write(vtkAbstractArray & array)
{
    return write({ { &array } });
}

bool TextFileWriter::write(
    const std::vector<vtkSmartPointer<vtkAbstractArray>> & arrays,
    const std::vector<vtkIdType> & rows)
{
    if (arrays.empty())
    {
        return true;
    }

    auto outTarget = output();
    if (!outTarget)
    {
        return false;
    }
    if (!outTarget->isOpen() || !outTarget->isWritable())
    {
        if (!outTarget->open(QIODevice::ReadWrite | QIODevice::Append))
        {
            qWarning() << "TextFileWriter: Cannot write to supplied file";
            return false;
        }
    }
    return m_writer->writeArrays(*outTarget, arrays, rows);
}

void swap(TextFileWriter & lhs, TextFileWriter & rhs)
{
    using std::swap;

    swap(lhs.m_outputFileName, rhs.m_outputFileName);
    swap(lhs.m_outputFile, rhs.m_outputFile);
    swap(lhs.m_outputTarget, rhs.m_outputTarget);
    swap(lhs.m_writer, rhs.m_writer);
}
