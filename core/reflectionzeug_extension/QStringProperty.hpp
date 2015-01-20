#pragma once

#include <core/reflectionzeug_extension/QStringProperty.h>


template <typename... Arguments>
QStringProperty::QStringProperty(Arguments&&... args)
    : ValueProperty<QString, reflectionzeug::StringPropertyInterface>(std::forward<Arguments>(args)...)
{
}


namespace reflectionzeug
{

    template <>
    struct PropertyClass<QString>
    {
        using Type = QStringProperty;
    };

}
