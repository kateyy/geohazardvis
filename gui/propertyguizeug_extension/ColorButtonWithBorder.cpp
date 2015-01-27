#include "ColorButtonWithBorder.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOption>


using namespace propertyguizeug;

ColorButtonWithBorder::ColorButtonWithBorder(QWidget * parent, const QColor & initialColor)
    : ColorButton{ parent, initialColor }
{
}

void ColorButtonWithBorder::paint(QPainter * painter, const QPoint & topLeft, const QColor & color)
{
    const auto metrics = painter->fontMetrics();
    const auto size = sizeFromFontHeight(metrics.height());
    const auto rect = QRect{ topLeft, size };

    auto pixmap = QPixmap{ size };
    pixmap.fill(Qt::white);

    QStyleOptionButton opt;
    opt.state = QStyle::State_Active | QStyle::State_Enabled;
    opt.rect = rect;

    QApplication::style()->drawControl(QStyle::CE_PushButton, &opt, painter);

    QSize innserSize = size - QSize(6, 6);

    QPixmap innerPixmap(innserSize);
    innerPixmap.fill(color);

    QRect innerRect(topLeft + QPoint(3, 3), innserSize);

    painter->save();
    painter->setBrushOrigin(topLeft);
    painter->fillRect(rect, Qt::BrushStyle::SolidPattern);
    painter->drawPixmap(rect, pixmap);
    painter->fillRect(innerRect, Qt::BrushStyle::SolidPattern);
    painter->drawPixmap(innerRect, innerPixmap);
    painter->restore();
}
