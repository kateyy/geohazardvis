#pragma once

#include <core/reflectionzeug_extension/QStringProperty.h>


namespace reflectionzeug
{

template <>
struct PropertyClass<QString>
{
    using Type = QStringProperty;
};

}
