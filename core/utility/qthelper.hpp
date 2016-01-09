#pragma once

#include "qthelper.h"

template<typename T, int Size>
QString vtkVectorToQString(const vtkVector<T, Size> & vector, 
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
