#include "PropertyEditorFactoryEx.h"

#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


void PropertyEditorFactoryEx::visit(reflectionzeug::ColorPropertyInterface * property)
{
    m_editor = new ColorEditorRGB(property);
}
