#include <core/utility/conversions.h>


template<typename FPType, size_t Size>
QString arrayToString(const std::array<FPType, Size> & a)
{
    if (Size == 0)
        return{};

    QString result;
    for (auto v : a)
        result += QString::number(v) + " ";
    result.truncate(result.length() - 1);

    return result;
}

template<typename FPType, size_t Size>
void stringToArray(const QString & s, std::array<FPType, Size> & resultArray)
{
    auto parts = s.split(" ");
    if (parts.size() < Size)
    {
        resultArray = {};
        return;
    }

    for (int i = 0; i < Size; ++i)
    {
        resultArray[i] = static_cast<FPType>(parts[i].toDouble());
    }
}

template<typename FPType, size_t Size>
std::array<FPType, Size> stringToArray(const QString & s)
{
    std::array<FPType, Size> result = {};
    stringToArray(s, result);
    return result;
}

template<typename FPType>
void stringToVector3(const QString & s, vtkVector3<FPType> & resultVector)
{
    auto array = stringToArray<FPType, 3u>(s);

    if (array.size() != 3u)
    {
        resultVector = {};
        return;
    }

    resultVector.Set(array[0], array[1], array[2]);
}

template<typename FPType>
vtkVector3<FPType> stringToVector3(const QString & s)
{
    vtkVector3<FPType> result;

    stringToVector3(s, result);

    return result;
}

template<typename FPType>
QString vector3ToString(const vtkVector3<FPType> & v)
{
    std::array<FPType, 3u> a;

    for (size_t i = 0; i < 3u; ++i)
    {
        a[i] = v[static_cast<int>(i)];
    }

    return arrayToString(a);
}

template<typename T, int Size>
QString vectorToString(const vtkVector<T, Size> & vector,
    const QString & separator, const QString & prefix, const QString & suffix)
{
    QString s;
    for (int i = 0; i < Size; ++i)
    {
        s += prefix + QString::number(vector[i]) + suffix + separator;
    }

    s.remove(s.length() - separator.length(), separator.length());

    return s;
}
