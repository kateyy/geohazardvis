#pragma once

#include <core/core_api.h>
#include <core/io/types.h>


class CORE_API TextFileReader
{
public:
    struct Result
    {
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

        Result(StateFlags stateFlags, size_t filePos = {});

        StateFlags stateFlags = StateFlag::unset;
        /** Byte position in the file after reading the last complete line.
          * This value is unspecified if State::eof is set. */
        size_t filePos = 0;
    };

    static Result read(
        const QString & inputFileName,
        io::InputVector & ioVectors,
        size_t offset = {},
        size_t numberOfLines = {});

    struct CORE_API Impl
    {
        static Result qFile_QByteArray(const QString & inputFileName,
            io::InputVector & ioVectors, size_t offset, size_t numberOfLines);
    };

    TextFileReader() = delete;
    ~TextFileReader() = delete;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TextFileReader::Result::StateFlags);
