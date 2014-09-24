#include "DataSetHandler.h"

#include <cassert>

#include <QList>

#include <core/data_objects/AttributeVectorData.h>


namespace
{
QList<DataObject *> * s_dataSets = nullptr;
QList<AttributeVectorData *> * s_attributeVectors = nullptr;
}


DataSetHandler::DataSetHandler()
    : QObject()
{
    s_dataSets = new QList<DataObject *>();
    s_attributeVectors = new QList<AttributeVectorData *>();
}

DataSetHandler::~DataSetHandler()
{
    qDeleteAll(dataSets());
    delete s_dataSets;
    qDeleteAll(attributeVectors());
    delete s_attributeVectors;
}

DataSetHandler & DataSetHandler::instance()
{
    static DataSetHandler instance;

    return instance;
}

void DataSetHandler::addData(QList<DataObject *> dataObjects)
{
    bool dataChanged = false, attributesChanged = false;
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
            assert(dynamic_cast<AttributeVectorData*>(dataObject));
            s_attributeVectors->append(static_cast<AttributeVectorData *>(dataObject));
            attributesChanged = true;
        }
    }

    if (dataChanged)
        emit dataObjectsChanged();
    if (attributesChanged)
        emit attributeVectorsChanged();
}

void DataSetHandler::deleteData(QList<DataObject *> dataObjects)
{
    bool dataChanged = false, attributesChanged = false;
    for (DataObject * dataObject : dataObjects)
    {
        if (dataObject->dataSet())
        {
            dataChanged = true;
            s_dataSets->removeOne(dataObject);
        }
        else
        {
            attributesChanged = true;
            s_attributeVectors->removeOne(static_cast<AttributeVectorData *>(dataObject));
        }

    }

    if (dataChanged)
        emit dataObjectsChanged();
    if (attributesChanged)
        emit attributeVectorsChanged();

    qDeleteAll(dataObjects);
}

const QList<DataObject *> & DataSetHandler::dataSets()
{
    return *s_dataSets;
}

const QList<AttributeVectorData *> & DataSetHandler::attributeVectors()
{
    return *s_attributeVectors;
}
