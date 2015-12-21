#include "BinaryFile.h"


template<typename T>
bool BinaryFile::write(const std::vector<T> & data)
{
    return write(data.data(), data.size() * sizeof(T));
}

template<typename T>
bool BinaryFile::read(size_t numValues, std::vector<T> & data)
{
    data.resize(numValues);

    const auto numBytesRead = read(numValues * sizeof(T), data.data());

    const auto numValuesRead = numBytesRead / sizeof(T);

    if (numValuesRead != numValues)
    {
        data.resize(numValuesRead);
        return false;
    }

    return true;
}
