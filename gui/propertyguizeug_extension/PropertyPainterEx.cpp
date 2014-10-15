#include "PropertyPainterEx.h"

#include <QApplication>
#include <QSpinBox>

#include <reflectionzeug/Property.h>
#include <propertyguizeug/ColorButton.h>


using namespace reflectionzeug;
using namespace propertyguizeug;


void PropertyPainterEx::visit(Property<Color> * property)
{
    this->drawItemViewBackground();

    const Color & color = property->value();

    QColor qcolor(color.red(),
        color.green(),
        color.blue(),
        color.alpha());
    QPoint topLeft(m_option.rect.left(),
        m_option.rect.top() + 4);

    ColorButton::paint(m_painter, topLeft, qcolor);

    QRect rect = m_option.rect;
    rect.setLeft(m_option.rect.left() +
        ColorButton::s_fixedSize.width() + 4);

    QString colorString = "R: " + QString::number(color.red()) + " G: " + QString::number(color.green()) + " B: " + QString::number(color.blue());

    const QWidget * widget = m_option.widget;
    QStyle * style = widget ? widget->style() : QApplication::style();
    style->drawItemText(m_painter,
        rect,
        Qt::AlignVCenter,
        m_option.palette,
        true,
        colorString,
        QPalette::Text);

    m_drawn = true;
}
