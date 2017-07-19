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

#pragma once

#include <QGroupBox>

#if COMPILE_QT_DESIGNER_PLUGIN
#define GUI_API
#else
#include <gui/gui_api.h>
#endif


class GUI_API CollapsibleGroupBox : public QGroupBox
{
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)

public:
    explicit CollapsibleGroupBox(QWidget * parent = nullptr);
    explicit CollapsibleGroupBox(const QString &title, QWidget * parent = nullptr);
    ~CollapsibleGroupBox() override;

    bool isCollapsed() const;

    void setCollapsed(bool collapsed);
    bool toggleCollapsed();

protected:
    void mouseReleaseEvent(QMouseEvent * event) override;

private:
    void applyCollapsed(bool collapsed);

private:
    Q_DISABLE_COPY(CollapsibleGroupBox)
};
