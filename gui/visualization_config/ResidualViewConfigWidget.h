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

#include <memory>
#include <vector>

#include <QWidget>

#include <gui/gui_api.h>


class Ui_ResidualViewConfigWidget;
class ResidualVerificationView;


class GUI_API ResidualViewConfigWidget : public QWidget
{
public:
    explicit ResidualViewConfigWidget(QWidget * parent = nullptr);
    ~ResidualViewConfigWidget() override;

    void setCurrentView(ResidualVerificationView * view);

private:
    void updateComboBoxes();

    void updateObservationFromUi(int index);
    void updateModelFromUi(int index);

private:
    std::unique_ptr<Ui_ResidualViewConfigWidget> m_ui;

    ResidualVerificationView * m_currentView;
    std::vector<QMetaObject::Connection> m_viewConnects;

private:
    Q_DISABLE_COPY(ResidualViewConfigWidget)
};
