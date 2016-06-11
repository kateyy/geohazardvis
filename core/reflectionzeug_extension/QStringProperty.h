#pragma once

#include <QString>

#include <reflectionzeug/StringPropertyInterface.h>
#include <reflectionzeug/Property.h>

#include <core/core_api.h>


class CORE_API QStringProperty : public reflectionzeug::ValueProperty<QString, reflectionzeug::StringPropertyInterface>
{
public:
    using Type = QString;

public:
    ~QStringProperty() override = 0;

    void accept(reflectionzeug::AbstractPropertyVisitor * visitor) override;

    std::string toString() const override;
    bool fromString(const std::string & string) override;
};


#include <core/reflectionzeug_extension/QStringProperty.hpp>
