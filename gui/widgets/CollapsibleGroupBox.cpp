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

#include "CollapsibleGroupBox.h"

#include <QMouseEvent>
#include <QStyle>


CollapsibleGroupBox::CollapsibleGroupBox(QWidget * parent)
    : CollapsibleGroupBox("", parent)
{
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget * parent)
    : QGroupBox(title, parent)
{
    setAttribute(Qt::WA_SetStyle);
    setCheckable(true);

    setStyleSheet(
        "QGroupBox::indicator {"
        "width: 13px;"
        "height: 13px;"
        "}"

        "QGroupBox::indicator:checked {"
        "image: url(:/icons/plus_green.svg);"
        "}"

        "QGroupBox::indicator:unchecked {"
        "image: url(:/icons/minus_green.svg);"
        "}"
        );

    connect(this, &QGroupBox::toggled, this, &CollapsibleGroupBox::applyCollapsed);
}

CollapsibleGroupBox::~CollapsibleGroupBox() = default;

bool CollapsibleGroupBox::isCollapsed() const
{
    return isChecked();
}

void CollapsibleGroupBox::setCollapsed(bool collapsed)
{
    setChecked(collapsed);
}

bool CollapsibleGroupBox::toggleCollapsed()
{
    setCollapsed(!isCollapsed());

    return isCollapsed();
}

void CollapsibleGroupBox::applyCollapsed(bool collapsed)
{
    if (collapsed)
    {
        setFixedHeight(QWIDGETSIZE_MAX);
        adjustSize();
    }
    else
    {
        setFixedHeight(17);
    }
}

void CollapsibleGroupBox::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->pos().y() > 20)
    {
        return;
    }

    toggleCollapsed();
}
