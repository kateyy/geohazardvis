#pragma once

#include <propertyguizeug/PropertyPainter.h>


class PropertyPainterEx : public propertyguizeug::PropertyPainter
{
protected:
    void visit(reflectionzeug::Property<reflectionzeug::Color> * property) override;
};
