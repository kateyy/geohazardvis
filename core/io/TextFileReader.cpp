#include <core/io/TextFileReader.h>

#include <limits>

#include <QFile>


TextFileReader::Result::Result(StateFlags stateFlags, size_t filePos)
    : stateFlags{ stateFlags }
    , filePos{ filePos }
{
}

auto TextFileReader::Impl::qFile_QByteArray(const QString & inputFileName,
    io::InputVector & ioVectors,
    size_t stdOffset,
    size_t numberOfLines) -> Result
{
    QFile file(inputFileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        return Result(Result::invalidFile);
    }

    using Offset_t = decltype(file.pos());
    const auto offset = static_cast<Offset_t>(stdOffset);

    if (stdOffset > static_cast<decltype(stdOffset)>(std::numeric_limits<Offset_t>::max()))
    {
        return Result(Result::invalidOffset);
    }

    // already at end of file
    if (offset > file.size())
    {
        return Result(Result::invalidOffset | Result::eof);
    }

    if (!file.seek(offset))
    {
        return Result(Result::invalidOffset | Result::eof);
    }

    io::t_FP input_FP;
    ioVectors.clear();
    
    size_t numberOfReadLines = 0;
    // allow to skip empty lines in the beginning
    bool assumeStarted = false;
    // Assume empty lines only at the end of the file, not in between data lines
    bool assumeAtEnd = false;

    while (!file.atEnd())
    {
        auto line = file.readLine();
        size_t choppedChars = 0;
        for (int i = 1; i <= 2 && i <= line.size(); ++i)
        {
            const auto c = line.at(line.size() - i);
            if (c == '\n' || c == '\r')
            {
                ++choppedChars;
            }
        }
        line.chop(static_cast<int>(choppedChars));

        const auto items = line.split(' ');

        size_t currentColumn = 0;

        for (int i = 0; i < items.size(); ++i)
        {
            auto & input = items[i];

            if (input.isEmpty())
            {
                continue;
            }
            if (assumeAtEnd)
            {
                // don't expect data after once reading an empty line
                return Result(Result::mismatchingColumnCount | (file.atEnd() ? Result::eof : Result::unset), file.pos());
            }
            if (input == "NaN")
            {
                input_FP = std::numeric_limits<io::t_FP>::quiet_NaN();
            }
            else
            {
                bool validConversion = false;
                if (std::is_same<io::t_FP, float>::value)
                {
                    input_FP = input.toFloat(&validConversion);
                }
                else
                {
                    input_FP = input.toDouble(&validConversion);
                }
                if (!validConversion)
                {
                    return Result(Result::invalidValue | (file.atEnd() ? Result::eof : Result::unset), file.pos());
                }
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
                    return Result(Result::mismatchingColumnCount, file.pos());
                }
            }

            ioVectors[currentColumn].push_back(input_FP);

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

        const auto eofFlag = file.atEnd() ? Result::eof : Result::unset;

        // don't allow later lines to have less values than the first line
        if (!assumeAtEnd && currentColumn != ioVectors.size())
        {
            return Result(Result::mismatchingColumnCount | eofFlag, file.pos());
        }

        if (numberOfLines != 0 && numberOfReadLines == numberOfLines)
        {
            return Result(Result::successful | eofFlag, file.pos());
        }
    }

    if (numberOfLines != 0 && numberOfLines != numberOfLines)
    {
        return Result(Result::eof, file.pos());
    }

    return Result(Result::successful | (file.atEnd() ? Result::eof : Result::unset), file.pos());
}

auto TextFileReader::read(
    const QString & inputFileName,
    io::InputVector & ioVectors,
    size_t stdOffset,
    size_t numberOfLines) -> Result
{
    return Impl::qFile_QByteArray(inputFileName, ioVectors, stdOffset, numberOfLines);
}
