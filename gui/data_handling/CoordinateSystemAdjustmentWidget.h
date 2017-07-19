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

#include <QDialog>
#include <QPointer>
#include <QVector>

#include <gui/gui_api.h>


class QMenu;

class CoordinateTransformableDataObject;
struct ReferencedCoordinateSystemSpecification;
class Ui_CoordinateSystemAdjustmentWidget;


class GUI_API CoordinateSystemAdjustmentWidget : public QDialog
{
public:
    explicit CoordinateSystemAdjustmentWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~CoordinateSystemAdjustmentWidget() override;

    void setDataObject(CoordinateTransformableDataObject * dataObject);

private:
    void updateInfoText();
    void finish();

    void setupQuickSetActions();

    ReferencedCoordinateSystemSpecification specFromUi() const;
    void specToUi(const ReferencedCoordinateSystemSpecification & spec);

private:
    std::unique_ptr<Ui_CoordinateSystemAdjustmentWidget> m_ui;
    std::unique_ptr<QMenu> m_autoSetReferencePointMenu;
    QVector<QAction *> m_refPointQuickSetActions;

    QPointer<CoordinateTransformableDataObject> m_dataObject;

private:
    Q_DISABLE_COPY(CoordinateSystemAdjustmentWidget)
};
