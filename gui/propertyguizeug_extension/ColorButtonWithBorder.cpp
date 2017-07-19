/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ColorButtonWithBorder.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOption>


using namespace propertyguizeug;

ColorButtonWithBorder::ColorButtonWithBorder(QWidget * parent, const QColor & initialColor)
    : ColorButton{ parent, initialColor }
{
}

ColorButtonWithBorder::~ColorButtonWithBorder() = default;

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
