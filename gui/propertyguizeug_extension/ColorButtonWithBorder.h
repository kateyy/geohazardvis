#pragma once

#include <propertyguizeug/ColorButton.h>

#include <gui/gui_api.h>


class ColorButtonWithBorder : public propertyguizeug::ColorButton
{
public:
    ColorButtonWithBorder(QWidget * parent = nullptr, const QColor & initialColor = Qt::black);

    static void paint(QPainter * painter, const QPoint & topLeft, const QColor & color);

protected:
    void paintEvent(QPaintEvent * event) override;
};
