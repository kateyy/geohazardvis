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

#include "DataSetHandler.h"

#include <cassert>
#include <mutex>

#include <QList>
#include <QMap>

#include <core/data_objects/RawVectorData.h>
#include <core/utility/memory.h>


using std::unique_ptr;
using std::vector;


class DataSetHandlerPrivate
{
public:
    DataSetHandlerPrivate()
    {
    }

    std::mutex mutex;

    vector<unique_ptr<DataObject>> dataSets;
    vector<unique_ptr<DataObject>> rawVectors;

    QList<DataObject *> externalDataSets;
    QList<RawVectorData *> externalRawVectors;

    /** convenience maps for the public interface */
    QList<DataObject *> allDataSets;
    QList<RawVectorData *> allRawVectors;
    QMap<DataObject *, bool> dataSetOwnerships;
    QMap<RawVectorData *, bool> rawVectorOwnerships;
};


DataSetHandler::DataSetHandler()
    : QObject()
    , d_ptr{ std::make_unique<DataSetHandlerPrivate>() }
{
}

DataSetHandler::~DataSetHandler() = default;

void DataSetHandler::cleanup()
{
    QList<DataObject *> ownedData;
    for (auto && data : d_ptr->dataSets)
    {
        ownedData << data.get();
    }
    deleteData(ownedData);
}

DataObject * DataSetHandler::takeData(std::unique_ptr<DataObject> dataObject)
{
    // passing by value requires callers to use "unique_ptr<..> obj; takeData(move(obj));"
    // thus, it's more explicit "move" on the caller's side

    DataObject * rawPtr = dataObject.get();

    // initializer list don't support move semantics, so we need a push_back(move...)) here
    std::vector<std::unique_ptr<DataObject>> vec;
    vec.push_back(std::move(dataObject));
    takeData(move(vec));

    return rawPtr;
}

void DataSetHandler::takeData(std::vector<std::unique_ptr<DataObject>> dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;

    {
        std::lock_guard<std::mutex>(d_ptr->mutex);
        for (auto && dataObject : dataObjects)
        {
            assert(dataObject);
            auto dataObjPtr = dataObject.get();

            // check special case: raw vector data
            if (auto rawData = dynamic_cast<RawVectorData *>(dataObjPtr))
            {
                // we can't already own this object (this would violate the unique_ptr concept)
                assert(!containsUnique(d_ptr->rawVectors, dataObjPtr));

                auto ownershipIt = d_ptr->rawVectorOwnerships.find(rawData);
                // check if we already list this object as external data
                if (ownershipIt != d_ptr->rawVectorOwnerships.end())
                {
                    assert(!ownershipIt.value());
                    assert(d_ptr->allRawVectors.count(rawData) == 1);
                    assert(d_ptr->externalRawVectors.count(rawData) == 1);

                    d_ptr->externalRawVectors.removeOne(rawData);
                }
                else    // new object, make sure to have valid list contents
                {
                    assert(!d_ptr->allRawVectors.contains(rawData));
                    assert(!d_ptr->externalRawVectors.contains(rawData));

                    d_ptr->allRawVectors << rawData;
                }

                d_ptr->rawVectorOwnerships.insert(rawData, true);
                d_ptr->rawVectors.push_back(std::move(dataObject));
                rawDataChanged = true;

                continue;
            }


            // assume valid DataObject, not a RawVectorData
            assert(dataObject->dataSet());

            // we can't already own this object (this would violate the unique_ptr concept)
            assert(!containsUnique(d_ptr->dataSets, dataObjPtr));

            auto ownershipIt = d_ptr->dataSetOwnerships.find(dataObjPtr);
            // check if we already list this object as external data
            if (ownershipIt != d_ptr->dataSetOwnerships.end())
            {
                assert(!ownershipIt.value());
                assert(d_ptr->allDataSets.count(dataObjPtr) == 1);
                assert(d_ptr->externalDataSets.count(dataObjPtr) == 1);

                d_ptr->externalDataSets.removeOne(dataObjPtr);
            }
            else    // new object, make sure to have valid list contents
            {
                assert(!d_ptr->allDataSets.contains(dataObjPtr));
                assert(!d_ptr->externalDataSets.contains(dataObjPtr));

                d_ptr->allDataSets << dataObjPtr;
            }

            d_ptr->dataSetOwnerships.insert(dataObjPtr, true);
            d_ptr->dataSets.push_back(std::move(dataObject));
            dataChanged = true;
        }
    }

    if (dataChanged)
    {
        emit dataObjectsChanged();
    }
    if (rawDataChanged)
    {
        emit rawVectorsChanged();
    }
}

void DataSetHandler::deleteData(const QList<DataObject *> & dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;

    std::vector<std::unique_ptr<DataObject>> dataToDelete;

    {
        std::lock_guard<std::mutex>(d_ptr->mutex);
        for (auto dataObject : dataObjects)
        {
            // deleteData might be called multiple times for fast interactions,
            // so check if we already processed this request.
            // Note: we cannot use dynamic_cast here to determine object types, as the object
            // might already be deleted.

            bool isRawVector = false;

            if (!d_ptr->allDataSets.contains(dataObject))
            {
                if (d_ptr->allRawVectors.contains(static_cast<RawVectorData *>(dataObject)))
                {
                    isRawVector = true;
                }
                else
                {
                    continue;
                }
            }

            if (isRawVector)
            {
                auto rawData = static_cast<RawVectorData *>(dataObject);
                auto it = findUnique(d_ptr->rawVectors, dataObject);
                // we can only delete data that we own
                if (it == d_ptr->rawVectors.end())
                {
                    continue;
                }

                d_ptr->rawVectors.erase(it);
                d_ptr->allRawVectors.removeOne(rawData);
                d_ptr->rawVectorOwnerships.remove(rawData);
                rawDataChanged = true;
                continue;
            }

            auto it = findUnique(d_ptr->dataSets, dataObject);
            // we can only delete data that we own
            if (it == d_ptr->dataSets.end())
            {
                continue;
            }

            dataToDelete.push_back(std::move(*it));
            d_ptr->dataSets.erase(it);
            d_ptr->allDataSets.removeOne(dataObject);
            d_ptr->dataSetOwnerships.remove(dataObject);
            assert(!d_ptr->externalDataSets.contains(dataObject));
            dataChanged = true;
        }
    }

    if (dataChanged)
    {
        emit dataObjectsChanged();
    }
    if (rawDataChanged)
    {
        emit rawVectorsChanged();
    }
}

void DataSetHandler::addExternalData(const QList<DataObject*>& dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;

    {
        std::lock_guard<std::mutex>(d_ptr->mutex);
        for (auto & dataObject : dataObjects)
        {
            assert(dataObject);

            // check special case: raw vector data
            if (auto rawData = dynamic_cast<RawVectorData *>(dataObject))
            {
                // we can't already own this object (this would violate the unique_ptr concept)
                assert(!containsUnique(d_ptr->rawVectors, dataObject));

                auto ownershipIt = d_ptr->rawVectorOwnerships.find(rawData);
                // check if we already list this object as external data
                if (ownershipIt != d_ptr->rawVectorOwnerships.end())
                {
                    assert(!ownershipIt.value());
                    assert(d_ptr->allRawVectors.count(rawData) == 1);
                    assert(d_ptr->externalRawVectors.count(rawData) == 1);

                    // fine, so nothing to do
                    continue;
                }

                // new object

                assert(!d_ptr->allRawVectors.contains(rawData));
                assert(!d_ptr->externalRawVectors.contains(rawData));

                d_ptr->allRawVectors << rawData;
                d_ptr->externalRawVectors << rawData;
                d_ptr->rawVectorOwnerships.insert(rawData, false);
                rawDataChanged = true;

                continue;
            }


            // assume valid DataObject, not a RawVectorData
            assert(dataObject->dataSet());

            // we can't already own this object (this would violate the unique_ptr concept)
            assert(!containsUnique(d_ptr->dataSets, dataObject));

            auto ownershipIt = d_ptr->dataSetOwnerships.find(dataObject);
            // check if we already list this object as external data
            if (ownershipIt != d_ptr->dataSetOwnerships.end())
            {
                assert(!ownershipIt.value());
                assert(d_ptr->allDataSets.count(dataObject) == 1);
                assert(d_ptr->externalDataSets.count(dataObject) == 1);

                // fine, so nothing to do
                continue;
            }

            // new object

            assert(!d_ptr->allDataSets.contains(dataObject));
            assert(!d_ptr->externalDataSets.contains(dataObject));

            d_ptr->allDataSets << dataObject;
            d_ptr->externalDataSets << dataObject;
            d_ptr->dataSetOwnerships.insert(dataObject, false);
            dataChanged = true;
        }
    }

    if (dataChanged)
    {
        emit dataObjectsChanged();
    }
    if (rawDataChanged)
    {
        emit rawVectorsChanged();
    }
}

void DataSetHandler::removeExternalData(const QList<DataObject*>& dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;

    {
        std::lock_guard<std::mutex>(d_ptr->mutex);
        for (DataObject * dataObject : dataObjects)
        {
            if (auto rawData = dynamic_cast<RawVectorData *>(dataObject))
            {
                assert(!containsUnique(d_ptr->rawVectors, dataObject));

                if (!d_ptr->externalRawVectors.removeOne(rawData))
                {
                    continue;
                }

                d_ptr->allRawVectors.removeOne(rawData);
                d_ptr->rawVectorOwnerships.remove(rawData);
                rawDataChanged = true;
                continue;
            }


            assert(!containsUnique(d_ptr->dataSets, dataObject));

            if (!d_ptr->externalDataSets.removeOne(dataObject))
            {
                continue;
            }

            d_ptr->allDataSets.removeOne(dataObject);
            d_ptr->dataSetOwnerships.remove(dataObject);
            dataChanged = true;
        }
    }

    if (dataChanged)
    {
        emit dataObjectsChanged();
    }
    if (rawDataChanged)
    {
        emit rawVectorsChanged();
    }
}

const QList<DataObject *> & DataSetHandler::dataSets() const
{
    std::lock_guard<std::mutex>(d_ptr->mutex);

    return d_ptr->allDataSets;
}

const QList<RawVectorData *> & DataSetHandler::rawVectors() const
{
    std::lock_guard<std::mutex>(d_ptr->mutex);

    return d_ptr->allRawVectors;
}

const QMap<DataObject*, bool>& DataSetHandler::dataSetOwnerships() const
{
    std::lock_guard<std::mutex>(d_ptr->mutex);

    return d_ptr->dataSetOwnerships;
}

const QMap<RawVectorData*, bool>& DataSetHandler::rawVectorOwnerships() const
{
    std::lock_guard<std::mutex>(d_ptr->mutex);

    return d_ptr->rawVectorOwnerships;
}

bool DataSetHandler::ownsData(DataObject * data)
{
    if (auto rawVector = dynamic_cast<RawVectorData*>(data))
    {
        return rawVectorOwnerships().value(rawVector, false);
    }

    return dataSetOwnerships().value(data, false);
}
