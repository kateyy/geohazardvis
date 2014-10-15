#include "ColorButtonWithBorder.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>


using namespace propertyguizeug;

ColorButtonWithBorder::ColorButtonWithBorder(QWidget * parent, const QColor & initialColor)
    : ColorButton(parent, initialColor)
{
}

void ColorButtonWithBorder::paint(QPainter * painter, const QPoint & topLeft, const QColor & color)
{
    QStyleOptionButton opt;
    opt.state = QStyle::State_Active | QStyle::State_Enabled;
    opt.rect = QRect(topLeft, s_fixedSize);

    QApplication::style()->drawControl(QStyle::CE_PushButton, &opt, painter);

    QSize innserSize = s_fixedSize - QSize(6, 6);

    QPixmap pixmap(innserSize);
    pixmap.fill(color);

    QRect rect(topLeft + QPoint(3, 3), innserSize);

    painter->setBrushOrigin(topLeft);
    painter->fillRect(rect, Qt::BrushStyle::SolidPattern);
    painter->drawPixmap(rect, pixmap);
}

void ColorButtonWithBorder::paintEvent(QPaintEvent * event)
{
    QPixmap pixmap(s_fixedSize);
    pixmap.fill(Qt::white);
    setPixmap(pixmap);

    ColorButton::paintEvent(event);

    QPainter painter(this);
    paint(&painter, QPoint(0,0), m_color);

    event->accept();
}
