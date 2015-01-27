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
class ColorButtonWithBorder;


class GUI_API ColorEditorRGB : public propertyguizeug::PropertyEditor
{
public:
    using Type = reflectionzeug::ColorPropertyInterface;

    static void paint(QPainter * painter,
        const QStyleOptionViewItem & option,
        reflectionzeug::ColorPropertyInterface & property);

    ColorEditorRGB(reflectionzeug::ColorPropertyInterface * property, QWidget * parent = nullptr);
    ~ColorEditorRGB() override;

    void openColorPicker();
    void readUiColor(int);

protected:
    QColor qColor() const;
    void setQColor(const QColor & qcolor);
    void setColor(const reflectionzeug::Color & color);

protected:
    ColorButtonWithBorder * m_button;
    std::array<QSpinBox *, 3> m_spinBoxes;

    reflectionzeug::ColorPropertyInterface * m_property;
};
