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

#include <QWidget>


template<typename T> class QList;

class Ui_DataBrowser;
class AbstractRenderView;
class DataBrowserTableModel;
class DataObject;
class DataMapping;


class DataBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit DataBrowser(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~DataBrowser() override;

    void setDataMapping(DataMapping * dataMapping);

    void setSelectedData(DataObject * dataObject);
    void setSelectedData(const QList<DataObject *> & dataObjects);
    QList<DataObject *> selectedDataObjects() const;
    QList<DataObject *> selectedDataSets() const;
    QList<DataObject *> selectedAttributeVectors() const;

signals:
    void selectedDataChanged(DataObject * dataObject);

protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    void updateModelForFocusedView();
    void updateModel(AbstractRenderView * renderView);

    /** show and bring to front the table for selected objects */
    void showTable();
    /** change visibility of selected objects in the current render view */
    void changeRenderedVisibility(DataObject * clickedObject);

    void menuAssignDataToIndexes(const QPoint & position, DataObject * clickedData);

    /** unload selected objects, free all data/settings, close views if empty */
    void removeFile();

    void evaluateItemViewClick(const QModelIndex & index, const QPoint & position);

private:
    std::unique_ptr<Ui_DataBrowser> m_ui;
    DataBrowserTableModel * m_tableModel;
    DataMapping * m_dataMapping;

private:
    Q_DISABLE_COPY(DataBrowser)
};
