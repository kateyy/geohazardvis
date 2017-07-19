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

#include <vector>

#include <QAbstractTableModel>

#include <vtkType.h>

#include <core/core_api.h>


class DataObject;
enum class IndexType;


class CORE_API QVtkTableModel : public QAbstractTableModel
{
public:
    explicit QVtkTableModel(QObject * parent = nullptr);
    ~QVtkTableModel() override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setDataObject(DataObject * dataObject);
    DataObject * dataObject();

    vtkIdType hightlightItemId() const;
    /** @return General association of listed positions and attributes (points vs. cells) */
    virtual IndexType indexType() const = 0;
    void setHighlightItemId(vtkIdType id);

    /** @return cell/point/etc id for a table cell. This is the row by default. */
    virtual vtkIdType itemIdAt(const QModelIndex & index) const;


protected:
    virtual void resetDisplayData() = 0;

    /**
     * Rebuild all table contents.
     * Useful to be connected to modified/changed events and signals. This must not be called
     * inside data() etc. function that are called during table updates!
     */
    void rebuild();
    /** Add a Qt signal connection that will be disconnected in rebuild(). */
    void addDataObjectConnection(const QMetaObject::Connection & connection);

private:
    DataObject * m_dataObject;
    vtkIdType m_hightlightId;

    std::vector<QMetaObject::Connection> m_dataObjectConnections;

private:
    Q_DISABLE_COPY(QVtkTableModel)
};
