#include "DataSetHandler.h"

#include <cassert>

#include <QList>

#include <core/data_objects/RawVectorData.h>


namespace
{
QList<DataObject *> * s_dataSets = nullptr;
QList<RawVectorData *> * s_rawVectors = nullptr;
}


DataSetHandler::DataSetHandler()
    : QObject()
{
    s_dataSets = new QList<DataObject *>();
    s_rawVectors = new QList<RawVectorData *>();
}

DataSetHandler::~DataSetHandler()
{
    qDeleteAll(dataSets());
    delete s_dataSets;
    qDeleteAll(rawVectors());
    delete s_rawVectors;
}

DataSetHandler & DataSetHandler::instance()
{
    static DataSetHandler instance;

    return instance;
}

void DataSetHandler::addData(QList<DataObject *> dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;
    for (DataObject * dataObject : dataObjects)
    {
        assert(dataObject);
        assert(!s_dataSets->contains(dataObject));

        if (dataObject->dataSet())
        {
            s_dataSets->append(dataObject);
            dataChanged = true;
        }
        else
        {
            assert(dynamic_cast<RawVectorData*>(dataObject));
            s_rawVectors->append(static_cast<RawVectorData *>(dataObject));
            rawDataChanged = true;
        }
    }

    if (dataChanged)
        emit dataObjectsChanged();
    if (rawDataChanged)
        emit rawVectorsChanged();
}

void DataSetHandler::deleteData(QList<DataObject *> dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;
    for (DataObject * dataObject : dataObjects)
    {
        if (dataObject->dataSet())
        {
            dataChanged = true;
            s_dataSets->removeOne(dataObject);
        }
        else
        {
            rawDataChanged = true;
            s_rawVectors->removeOne(static_cast<RawVectorData *>(dataObject));
        }

    }

    if (dataChanged)
        emit dataObjectsChanged();
    if (rawDataChanged)
        emit rawVectorsChanged();

    qDeleteAll(dataObjects);
}

const QList<DataObject *> & DataSetHandler::dataSets()
{
    return *s_dataSets;
}

const QList<RawVectorData *> & DataSetHandler::rawVectors()
{
    return *s_rawVectors;
}
