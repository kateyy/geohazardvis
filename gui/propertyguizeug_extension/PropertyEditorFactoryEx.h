#pragma once

#include <propertyguizeug/PropertyEditorFactory.h>

#include <gui/gui_api.h>


class GUI_API PropertyEditorFactoryEx : public propertyguizeug::PropertyEditorFactory
{
protected:
    void visit(reflectionzeug::ColorPropertyInterface * property);
};
