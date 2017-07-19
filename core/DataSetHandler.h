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

#include <QObject>

#include <core/core_api.h>


template <typename T> class QList;
template <typename K, typename V> class QMap;

class DataSetHandlerPrivate;
class DataObject;
class RawVectorData;


class CORE_API DataSetHandler : public QObject
{
    Q_OBJECT

public:
    DataSetHandler();
    ~DataSetHandler() override;
    void cleanup();

    /** Add data objects to the global data management.
        The DataSetHandler will take ownership of this data, so it can also delete it at any time. */
    DataObject * takeData(std::unique_ptr<DataObject> dataObject);
    void takeData(std::vector<std::unique_ptr<DataObject>> dataObjects);
    void deleteData(const QList<DataObject *> & dataObjects);

    /** Add external data to the DataSetHandler, which will be exposed to other listeners.
        The DataSetHandler does not take ownership of this data , so it will never delete it. */
    void addExternalData(const QList<DataObject *> & dataObjects);
    void removeExternalData(const QList<DataObject *> & dataObjects);

    const QList<DataObject *> & dataSets() const;
    const QList<RawVectorData *> & rawVectors() const;

    const QMap<DataObject *, bool> & dataSetOwnerships() const;
    const QMap<RawVectorData *, bool> & rawVectorOwnerships() const;
    bool ownsData(DataObject * data);

signals:
    void dataObjectsChanged();
    void rawVectorsChanged();

private:
    friend class DataSetHandler_test;

    std::unique_ptr<DataSetHandlerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(DataSetHandler)
};
