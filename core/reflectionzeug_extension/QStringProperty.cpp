#include <core/reflectionzeug_extension/QStringProperty.h>

#include <reflectionzeug/PropertyVisitor.h>


QStringProperty::~QStringProperty() = default;

void QStringProperty::accept(reflectionzeug::AbstractPropertyVisitor * visitor)
{
    reflectionzeug::StringPropertyInterface::accept(visitor);
}

std::string QStringProperty::toString() const
{
    return std::string{ this->value().toUtf8().data() };
}

bool QStringProperty::fromString(const std::string & string)
{
    QString value = QString::fromUtf8(string.c_str());

    this->setValue(value);
    return true;
}
