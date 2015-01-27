#pragma once

#include <propertyguizeug/ColorButton.h>

#include <gui/gui_api.h>


namespace reflectionzeug
{
    class ColorPropertyInterface;
}

class GUI_API ColorButtonWithBorder : public propertyguizeug::ColorButton
{
public:
    using Type = reflectionzeug::ColorPropertyInterface;

    ColorButtonWithBorder(QWidget * parent = nullptr, const QColor & initialColor = Qt::black);

    static void paint(
        QPainter * painter,
        const QPoint & topLeft,
        const QColor & color);
};
