#include "DataSetHandler.h"

#include <cassert>

#include <QList>

#include <core/data_objects/DataObject.h>


namespace
{
QList<DataObject *> * s_dataObjects = nullptr;
}


DataSetHandler::DataSetHandler()
    : QObject()
{
    s_dataObjects = new QList<DataObject *>();
}

DataSetHandler::~DataSetHandler()
{
    qDeleteAll(dataObjects());
    delete s_dataObjects;
}

DataSetHandler & DataSetHandler::instance()
{
    static DataSetHandler instance;

    return instance;
}

void DataSetHandler::add(DataObject * dataObject)
{
    assert(dataObject);
    assert(!dataObjects().contains(dataObject));
    s_dataObjects->append(dataObject);
}

void DataSetHandler::deleteObject(DataObject * dataObject)
{
    s_dataObjects->removeOne(dataObject);
    delete dataObject;
}

QList<DataObject *> DataSetHandler::dataObjects()
{
    return *s_dataObjects;
}
