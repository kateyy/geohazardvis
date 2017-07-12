#pragma once

#include <memory>
#include <type_traits>

#include <core/core_api.h>


class QString;
class DataObject;


class CORE_API Loader
{
public:
    static std::unique_ptr<DataObject> readFile(const QString & filename);

    template<typename T> 
    static typename std::enable_if<std::is_base_of<DataObject, T>::value, std::unique_ptr<T>>::type
    readFile(const QString & fileName);

private:
    Loader() = delete;
    ~Loader() = delete;
};


#include "Loader.hpp"
