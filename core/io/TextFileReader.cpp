#include <core/io/TextFileReader.h>

#include <limits>

#include <QFile>


TextFileReader::Result::Result(State state, size_t filePos)
    : state{ state }
    , filePos{ filePos }
{
}

auto TextFileReader::Impl::qFile_QByteArray(const QString & inputFileName,
    io::InputVector & ioVectors,
    size_t stdOffset,
    size_t numberOfValues) -> Result
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
        return Result(Result::eof);
    }

    if (!file.seek(offset))
    {
        return Result(Result::invalidOffset);
    }

    io::t_FP input_FP;
    ioVectors.clear();

    size_t currentNumValues = 0;
    size_t filePos = 0;

    while (!file.atEnd())
    {
        auto line = file.readLine();
        size_t choppedChars = 0;
        for (int i = 1; i <= 2 && i <= line.size(); ++i)
        {
            const auto c = line[line.size() - i];
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

            if (i != 0)
            {
                ++filePos; // increment for split char before the current item
            }
            filePos += static_cast<size_t>(input.size());

            if (input.isEmpty())
            {
                continue;
            }
            if (input == "NaN")
            {
                input_FP = std::numeric_limits<io::t_FP>::quiet_NaN();
            }
            else
            {
                input_FP = input.toFloat();
            }

            if (ioVectors.size() <= currentColumn)
            {
                ioVectors.push_back({});
            }

            ioVectors[currentColumn].push_back(input_FP);

            ++currentNumValues;

            if (currentNumValues == numberOfValues)
            {
                return Result(Result::noError, filePos);
            }

            ++currentColumn;
        }

        filePos += choppedChars;
    }

    if (numberOfValues != 0 && currentNumValues != numberOfValues)
    {
        return Result(Result::eof, filePos);
    }

    return Result(Result::noError, filePos);
}

auto TextFileReader::read(
    const QString & inputFileName,
    io::InputVector & ioVectors,
    size_t stdOffset,
    size_t numberOfValues) -> Result
{
    return Impl::qFile_QByteArray(inputFileName, ioVectors, stdOffset, numberOfValues);
}
