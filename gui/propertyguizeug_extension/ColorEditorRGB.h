#pragma once

#include <array>

#include <reflectionzeug/property_declaration.h>
#include <propertyguizeug/PropertyEditor.h>

#include <gui/gui_api.h>


class QSpinBox;

namespace reflectionzeug
{
    class ColorPropertyInterface;
    class Color;
}
namespace propertyguizeug
{
    class ColorButton;
}


class GUI_API ColorEditorRGB : public propertyguizeug::PropertyEditor
{
public:
    ColorEditorRGB(reflectionzeug::ColorPropertyInterface * property, QWidget * parent = nullptr);
    ~ColorEditorRGB() override;

    void openColorPicker();
    void readUiColor(int);

protected:
    QColor qcolor() const;
    void setQColor(const QColor & qcolor);
    void setColor(const reflectionzeug::Color & color);

protected:
    propertyguizeug::ColorButton * m_button;
    std::array<QSpinBox *, 3> m_spinBoxes;

    reflectionzeug::ColorPropertyInterface * m_property;
};
