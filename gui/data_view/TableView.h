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

#include <gui/data_view/AbstractDataView.h>


class QTableView;
class QItemSelection;
class QMenu;

class Ui_TableView;
class QVtkTableModel;


class GUI_API TableView : public AbstractDataView
{
    Q_OBJECT

public:
    TableView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~TableView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);

    void showDataObject(DataObject & dataObject);
    DataObject * dataObject();

signals:
    void itemDoubleClicked(const DataSelection & selection);

protected:
    QWidget * contentWidget() override;
    void onSetSelection(const DataSelection & selection) override;
    void onClearSelection() override;

    std::pair<QString, std::vector<QString>> friendlyNameInternal() const override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    std::unique_ptr<Ui_TableView> m_ui;
    DataObject * m_dataObject;
    QMenu * m_selectColumnsMenu;

    QMetaObject::Connection m_hightlightUpdateConnection;

private:
    Q_DISABLE_COPY(TableView)
};
