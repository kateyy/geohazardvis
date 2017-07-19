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

#include <map>
#include <vector>

#include <QAbstractTableModel>


class QIcon;

class DataObject;
class DataSetHandler;


class DataBrowserTableModel : public QAbstractTableModel
{
public:
    explicit DataBrowserTableModel(QObject * parent = nullptr);
    ~DataBrowserTableModel() override;

    void setDataSetHandler(const DataSetHandler * dataSetHandler);

    void updateDataList(const QList<DataObject *> & visibleObjects);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    DataObject * dataObjectAt(int row) const;
    DataObject * dataObjectAt(const QModelIndex & index) const;
    int rowForDataObject(DataObject * dataObject) const;

    QList<DataObject *> dataObjects(QModelIndexList indexes);
    QList<DataObject *> dataSets(QModelIndexList indexes);
    QList<DataObject *> rawVectors(QModelIndexList indexes);

    static int numButtonColumns();

    /** nice name for each component (x, y, z for up to three components, [0].. for more) */
    static QString componentName(int component, int numComponents);

private:
    QVariant data_dataObject(int row, int column, int role) const;
    QVariant data_attributeVector(int row, int column, int role) const;

private:
    const DataSetHandler * m_dataSetHandler;
    std::map<const DataObject *, bool> m_visibilities;
    std::map<QString, QIcon> m_icons;
    std::vector<QMetaObject::Connection> m_dataObjectConnections;
    int m_numDataObjects;
    int m_numAttributeVectors;

private:
    Q_DISABLE_COPY(DataBrowserTableModel)
};
