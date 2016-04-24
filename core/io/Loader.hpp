#pragma once

#include "Loader.h"


template<typename T> typename std::enable_if<std::is_base_of<DataObject, T>::value, std::unique_ptr<T>>::type
Loader::readFile(const QString & fileName)
{
    auto genericDataSet = readFile(fileName);
    auto specificDataSet = dynamic_cast<T *>(genericDataSet.get());
    if (!specificDataSet)
    {
        return nullptr;
    }

    genericDataSet.release();

    return std::unique_ptr<T>(specificDataSet);
}
