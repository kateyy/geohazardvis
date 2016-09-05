#pragma once

#include <core/core_api.h>
#include <core/io/types.h>


class CORE_API TextFileReader
{
public:
    struct Result
    {
        enum State
        {
            noError,
            invalidFile,    // input file does not exist or is not readable
            // end of file was reached before filling all columns of the last row, or before
            // numberOfValues values were read (if specified)
            eof,
            invalidOffset   // offset points behind the end of the file or is too large of qint64
        };

        Result(State state, size_t filePos = {});

        State state;
        size_t filePos = 0;
    };

    static Result read(
        const QString & inputFileName,
        io::InputVector & ioVectors,
        size_t offset = {},
        size_t numberOfValues = {});

    struct CORE_API Impl
    {
        static Result qFile_QByteArray(const QString & inputFileName,
            io::InputVector & ioVectors, size_t offset, size_t numberOfValues);
    };
};
