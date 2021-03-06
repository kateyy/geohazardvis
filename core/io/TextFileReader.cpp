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

#include <core/io/TextFileReader.h>

#include <cassert>
#include <limits>
#include <type_traits>

#include <QDebug>
#include <QFile>

#include <core/utility/macros.h>


namespace
{

template <typename FlagsT, typename FlagT>
FlagsT & setFlags(FlagsT & flags, FlagT flagToSet)
{
    flags |= flagToSet;
    return flags;
}

template <typename FlagsT>
FlagsT & unsetFlags(FlagsT & flags, FlagsT flagsToUnset)
{
    flags &= ~flagsToUnset;
    return flags;
}

template <typename FlagsT, typename FlagT>
FlagsT & unsetFlag(FlagsT & flags, FlagT flagToUnset)
{
    flags &= ~FlagsT(flagToUnset);
    return flags;
}

}


struct ImplementationQt : public TextFileReader::ImplBase
{
    explicit ImplementationQt(TextFileReader & reader)
        : TextFileReader::ImplBase(reader)
        , file{}
    {
    }
    ~ImplementationQt() override = default;

    void setFileName(const QString & fileName) override
    {
        file.setFileName(fileName);
    }

    using StateFlag = TextFileReader::StateFlag;
    using StateFlags = TextFileReader::StateFlags;
    using FloatVectors = TextFileReader::FloatVectors;
    using DoubleVectors = TextFileReader::DoubleVectors;
    using StringVectors = TextFileReader::StringVectors;

    void read(FloatVectors & floatIOVectors, size_t numberOfLines = {}) override;
    void read(DoubleVectors & doubleIOVectors, size_t numberOfLines = {}) override;
    void read(StringVectors & stringIOVectors, size_t numberOfLines = {}) override;
    uint64_t filePos() override;
    void seekTo(uint64_t filePos) override;

    static StateFlags tryOpen(QFile & file);

private:
    QFile file;
};

namespace
{

std::unique_ptr<TextFileReader::ImplBase> instantiateImpl(
    TextFileReader & reader,
    TextFileReader::ImplementationID DEBUG_ONLY(id))
{
    assert(id == TextFileReader::ImplementationID::Qt);
    return std::make_unique<ImplementationQt>(reader);
}

}


TextFileReader::TextFileReader(const QString & fileName)
    : TextFileReader(ImplementationID::Qt, fileName)
{
}

TextFileReader::TextFileReader(ImplementationID implId, const QString & fileName)
    : m_implementation{ instantiateImpl(*this, implId) }
    , m_fileName{ fileName }
    , m_delimiter{ ' ' }
{
    m_implementation->setFileName(m_fileName);
}

TextFileReader::TextFileReader(std::nullptr_t)
    : m_implementation{}
    , m_fileName{}
    , m_delimiter{}
{
}

TextFileReader::~TextFileReader() = default;

void swap(TextFileReader & lhs, TextFileReader & rhs)
{
    using std::swap;

    swap(lhs.m_implementation, rhs.m_implementation);
    swap(lhs.m_fileName, rhs.m_fileName);
}

TextFileReader::TextFileReader(TextFileReader && other)
    : TextFileReader(nullptr)
{
    swap(*this, other);
}

TextFileReader & TextFileReader::operator=(TextFileReader && other)
{
    swap(*this, other);

    return *this;
}

const QString & TextFileReader::fileName() const
{
    return m_fileName;
}

void TextFileReader::setFileName(const QString & fileName)
{
    if (m_fileName == fileName)
    {
        return;
    }

    m_fileName = fileName;

    m_implementation->setFileName(fileName);
}

QChar TextFileReader::delimiter() const
{
    return m_delimiter;
}

void TextFileReader::setDelimiter(const QChar delimiter)
{
    m_delimiter = delimiter;
}

auto TextFileReader::stateFlags() const -> StateFlags
{
    return m_implementation->stateFlags();
}

uint64_t TextFileReader::filePos() const
{
    return m_implementation->filePos();
}

void TextFileReader::seekTo(uint64_t filePos)
{
    return m_implementation->seekTo(filePos);
}

auto TextFileReader::read(FloatVectors & floatIOVectors, size_t numberOfLines) -> StateFlags
{
    m_implementation->read(floatIOVectors, numberOfLines);
    return m_implementation->stateFlags();
}

auto TextFileReader::read(DoubleVectors & doubleIOVectors, size_t numberOfLines) -> StateFlags
{
    m_implementation->read(doubleIOVectors, numberOfLines);
    return m_implementation->stateFlags();
}

auto TextFileReader::read(StringVectors & stringIOVectors, size_t numberOfLines) -> StateFlags
{
    m_implementation->read(stringIOVectors, numberOfLines);
    return m_implementation->stateFlags();
}

TextFileReader::ImplBase::ImplBase(TextFileReader & reader)
    : m_reader{ reader }
    , m_stateFlags{ unset }
{
}

TextFileReader::ImplBase::~ImplBase() = default;

auto TextFileReader::ImplBase::stateFlags() const -> StateFlags
{
    return m_stateFlags;
}

namespace
{

template<typename T>
struct qFile_QByteArray_Worker
{
    using ValueType = T;
    using StateFlag = TextFileReader::StateFlag;
    using StateFlags = TextFileReader::StateFlags;

    static StateFlags read(
        QFile & file,
        QChar delimiter,
        std::vector<std::vector<T>> & ioVectors,
        size_t numberOfLines);

    static bool checkValue(const QStringRef & readValue, T & checkedValue);
};


template<typename T>
auto qFile_QByteArray_Worker<T>::read(
    QFile & file,
    const QChar delimiter,
    std::vector<std::vector<T>> & ioVectors,
    size_t numberOfLines) -> StateFlags
{
    StateFlags stateFlags = ImplementationQt::tryOpen(file);
    if (!stateFlags.testFlag(StateFlag::successful))
    {
        return stateFlags;
    }

    unsetFlag(stateFlags, StateFlag::successful);

    ValueType checkedValue;
    ioVectors.clear();

    size_t numberOfReadLines = 0;
    // allow to skip empty lines in the beginning
    bool assumeStarted = false;
    // Assume empty lines only at the end of the file, not in between data lines
    bool assumeAtEnd = false;

    // Treat multiple consecutive whitespace delimiters as a single delimiter. This does not apply
    // for all other delimiter characters.
    const auto splitBehavior = delimiter == ' '
        ? QString::SkipEmptyParts
        : QString::KeepEmptyParts;

    while (!file.atEnd())
    {
        const auto lineData = QString(file.readLine());
        // Remove whitespace from the start and end, including end-of-line characters.
        const auto line = QStringRef(&lineData).trimmed();
        // Split according delimiter/split specification.
        // QStringRef::split returns itself in a vector if its referenced string is empty.
        // This is not really useful for the following loop, so check that case.
        const auto items = line.isEmpty()
            ? decltype(line.split(delimiter, splitBehavior))()
            : line.split(delimiter, splitBehavior);

        size_t currentColumn = 0;

        for (auto && input : items)
        {
            assert(!input.isNull());
            if (assumeAtEnd)
            {
                // don't expect data after once reading an empty line
                return setFlags(stateFlags, StateFlag::mismatchingColumnCount
                    | (file.atEnd() ? StateFlag::eof : StateFlag::unset));
            }

            if (!checkValue(input, checkedValue))
            {
                return setFlags(stateFlags, StateFlag::invalidValue
                    | (file.atEnd() ? StateFlag::eof : StateFlag::unset));
            }

            if (ioVectors.size() <= currentColumn)
            {
                // When reading the first line, count the number of columns
                if (numberOfReadLines == 0)
                {
                    ioVectors.push_back({});
                }
                else
                {
                    // don't allow later lines to have more values than the first line
                    return setFlags(stateFlags, StateFlag::mismatchingColumnCount
                        | (file.atEnd() ? StateFlag::eof : StateFlag::unset));
                }
            }

            ioVectors[currentColumn].push_back(checkedValue);

            ++currentColumn;
        }

        if (!assumeStarted && currentColumn != 0)
        {
            assumeStarted = true;
        }

        if (assumeStarted)
        {
            if (currentColumn == 0)
            {
                assumeAtEnd = true;
            }
            else
            {
                ++numberOfReadLines;
            }
        }

        const auto eofFlag = file.atEnd() ? StateFlag::eof : StateFlag::unset;

        // don't allow later lines to have less values than the first line
        if (!assumeAtEnd && currentColumn != ioVectors.size())
        {
            return setFlags(stateFlags, StateFlag::mismatchingColumnCount | eofFlag);
        }

        if (numberOfLines != 0 && numberOfReadLines == numberOfLines)
        {
            return setFlags(stateFlags, StateFlag::successful | eofFlag);
        }
    }

    if (numberOfLines != 0 && numberOfLines != numberOfReadLines)
    {
        return setFlags(stateFlags, StateFlag::eof);
    }

    return setFlags(stateFlags, StateFlag::successful
        | (file.atEnd() ? StateFlag::eof : StateFlag::unset));
}


template<>
bool qFile_QByteArray_Worker<float>::checkValue(const QStringRef & readValue, ValueType & checkedValue)
{
    if (readValue == "NaN")
    {
        checkedValue = std::numeric_limits<ValueType>::quiet_NaN();
        return true;
    }
    bool validConversion = false;
    checkedValue = readValue.toFloat(&validConversion);
    return validConversion;
}

template<>
bool qFile_QByteArray_Worker<double>::checkValue(const QStringRef & readValue, ValueType & checkedValue)
{
    if (readValue == "NaN")
    {
        checkedValue = std::numeric_limits<ValueType>::quiet_NaN();
        return true;
    }
    bool validConversion = false;
    checkedValue = readValue.toDouble(&validConversion);
    return validConversion;
}

template<>
bool qFile_QByteArray_Worker<QString>::checkValue(const QStringRef & readValue, ValueType & checkedValue)
{
    checkedValue = readValue.toString();
    return true;
}

}

void ImplementationQt::read(FloatVectors & floatIOVectors, size_t numberOfLines)
{
    using ValueType = std::remove_reference_t<decltype(floatIOVectors)>::value_type::value_type;

    m_stateFlags = qFile_QByteArray_Worker<ValueType>::read(
        file, m_reader.delimiter(), floatIOVectors, numberOfLines);
}

void ImplementationQt::read(DoubleVectors & doubleIOVectors, size_t numberOfLines)
{
    using ValueType = std::remove_reference_t<decltype(doubleIOVectors)>::value_type::value_type;

    m_stateFlags = qFile_QByteArray_Worker<ValueType>::read(
        file, m_reader.delimiter(), doubleIOVectors, numberOfLines);
}

void ImplementationQt::read(StringVectors & stringIOVectors, size_t numberOfLines)
{
    using ValueType = std::remove_reference_t<decltype(stringIOVectors)>::value_type::value_type;

    m_stateFlags = qFile_QByteArray_Worker<ValueType>::read(
        file, m_reader.delimiter(), stringIOVectors, numberOfLines);
}

uint64_t ImplementationQt::filePos()
{
    const auto posInternal = std::max<decltype(file.pos())>(0, file.pos());
    const auto posResult = static_cast<decltype(filePos())>(posInternal);
    if (posInternal != static_cast<decltype(posInternal)>(posResult))
    {
        qWarning() << "Internal file position not representable in public interface: " << posInternal;
        return{};
    }

    return posResult;
}

void ImplementationQt::seekTo(uint64_t filePos)
{
    m_stateFlags = tryOpen(file);
    if (!m_stateFlags.testFlag(StateFlag::successful))
    {
        return;
    }

    using Offset_t = decltype(file.pos());
    const auto offset = static_cast<Offset_t>(filePos);

    if (filePos != static_cast<uint64_t>(offset))
    {
        unsetFlag(m_stateFlags, StateFlag::successful);
        setFlags(m_stateFlags, StateFlag::invalidOffset);
        qWarning() << "File offset exceeds representable values in current implementation:"
            << filePos << " (" << m_reader.fileName() << ")";
        return;
    }

    // already at end of file
    if (offset > file.size())
    {
        unsetFlag(m_stateFlags, StateFlag::successful);
        setFlags(m_stateFlags, StateFlag::invalidOffset | StateFlag::eof);
        return;
    }

    if (!file.seek(offset))
    {
        unsetFlag(m_stateFlags, StateFlag::successful);
        setFlags(m_stateFlags, StateFlag::invalidOffset | StateFlag::eof);
        return;
    }

    setFlags(m_stateFlags, StateFlag::successful);
}

auto ImplementationQt::tryOpen(QFile & file) -> StateFlags
{
    if ((file.isOpen() && file.isReadable()) || file.open(QIODevice::ReadOnly))
    {
        return StateFlag::successful;
    }

    return StateFlag::invalidFile;
}
