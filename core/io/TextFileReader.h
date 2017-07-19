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

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <QString>

#include <core/core_api.h>


class CORE_API TextFileReader
{
public:
    explicit TextFileReader(const QString & fileName = {});
    virtual ~TextFileReader();
    TextFileReader(TextFileReader && other);
    TextFileReader & operator=(TextFileReader && other);

    const QString & fileName() const;
    void setFileName(const QString & fileName);

    template<typename T>
    using Vector_t = std::vector<std::vector<T>>;
    using FloatVectors = Vector_t<float>;
    using DoubleVectors = Vector_t<double>;
    using StringVectors = Vector_t<QString>;

    enum StateFlag
    {
        unset = 0x0,
        successful = 0x1,
        // End of file was reached when returning. Check if this is combined with successful
        // or an error flag to check if the result contents are valid.
        eof = 0x2,
        // input file does not exist or is not readable
        invalidFile = 0x4,
        // end of file was reached before filling all columns of the last row, or before
        // numberOfLines complete lines were read (if specified)
        // the number of columns is not the same for all read rows
        mismatchingColumnCount = 0x8,
        // A value could not be converted to a floating point representation
        invalidValue = 0x10,
        // offset points behind the end of the file or is too large of qint64
        invalidOffset = 0x20
    };
    Q_DECLARE_FLAGS(StateFlags, StateFlag);

    StateFlags stateFlags() const;
    /** Byte position in the file after reading the last complete line.
      * This value is undefined if State::eof is set. */
    uint64_t filePos() const;
    void seekTo(uint64_t filePos);

    StateFlags read(FloatVectors & floatIOVectors, size_t numberOfLines = {});
    StateFlags read(DoubleVectors & doubleIOVectors, size_t numberOfLines = {});
    StateFlags read(StringVectors & stringIOVectors, size_t numberOfLines = {});


    friend void swap(TextFileReader & lhs, TextFileReader & rhs);

    enum class ImplementationID { Qt };
    class CORE_API ImplBase
    {
    public:
        explicit ImplBase(TextFileReader & reader);
        virtual ~ImplBase() = 0;

        virtual void setFileName(const QString & fileName) = 0;

        virtual void read(FloatVectors & floatIOVectors, size_t numberOfLines = {}) = 0;
        virtual void read(DoubleVectors & doubleIOVectors, size_t numberOfLines = {}) = 0;
        virtual void read(StringVectors & stringIOVectors, size_t numberOfLines = {}) = 0;
        virtual uint64_t filePos() = 0;
        virtual void seekTo(size_t filePos) = 0;

        StateFlags stateFlags() const;

    protected:
        TextFileReader & m_reader;
        StateFlags m_stateFlags;
    };

protected:
    explicit TextFileReader(ImplementationID implId, const QString & fileName);

private:
    explicit TextFileReader(std::nullptr_t);

private:
    std::unique_ptr<ImplBase> m_implementation;
    QString m_fileName;

private:
    Q_DISABLE_COPY(TextFileReader)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TextFileReader::StateFlags);
