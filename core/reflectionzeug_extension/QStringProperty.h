#pragma once

#include <QString>

#include <reflectionzeug/Property.h>

#include <core/core_api.h>


class QStringProperty : public reflectionzeug::ValueProperty<QString, reflectionzeug::StringPropertyInterface>
{
public:
    template <typename... Arguments>
    QStringProperty(Arguments&&... args);

    CORE_API std::string toString() const;

    CORE_API bool fromString(const std::string & string);
};


#include <core/reflectionzeug_extension/QStringProperty.hpp>
